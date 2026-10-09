using System.Collections.Immutable;
using System.IO.Compression;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Sloop.Workstation;

public sealed record CompositionArchiveDocument(int Version, SavedCompositionDraft Draft);
[JsonSourceGenerationOptions(UnmappedMemberHandling = JsonUnmappedMemberHandling.Disallow)]
[JsonSerializable(typeof(CompositionArchiveDocument))]
internal partial class CompositionArchiveJsonContext : JsonSerializerContext;

/// <summary>A single portable draft. Never extracts paths or changes the active pattern.</summary>
public static class CompositionDraftArchive
{
    public const int MaximumBytes = 4 * 1024 * 1024;
    const string EntryName = "draft.json";

    public static byte[] Export(SavedCompositionDraft draft)
    {
        CompositionDraftStore.Validate(new(1, [draft]));
        var payload = JsonSerializer.SerializeToUtf8Bytes(new CompositionArchiveDocument(1, draft), CompositionArchiveJsonContext.Default.CompositionArchiveDocument);
        if (payload.Length > MaximumBytes) throw new InvalidDataException("Draft payload exceeds size limit.");
        using var output = new MemoryStream();
        using (var zip = new ZipArchive(output, ZipArchiveMode.Create, true))
        using (var entry = zip.CreateEntry(EntryName, CompressionLevel.Optimal).Open()) entry.Write(payload);
        if (output.Length > MaximumBytes) throw new InvalidDataException("Draft archive exceeds size limit.");
        return output.ToArray();
    }

    // Read bounds apply to actual streamed bytes, including non-seekable SAF providers.
    static byte[] ReadBounded(Stream input)
    {
        using var output = new MemoryStream();
        var buffer = new byte[8192];
        int count;
        while ((count = input.Read(buffer, 0, Math.Min(buffer.Length, MaximumBytes + 1 - (int)output.Length))) != 0)
        {
            output.Write(buffer, 0, count);
            if (output.Length > MaximumBytes) throw new InvalidDataException("Draft archive or payload exceeds size limit.");
        }
        return output.ToArray();
    }

    static void RejectDuplicateProperties(JsonElement element)
    {
        if (element.ValueKind == JsonValueKind.Object)
        {
            var names = new HashSet<string>(StringComparer.Ordinal);
            foreach (var property in element.EnumerateObject())
            {
                if (!names.Add(property.Name)) throw new InvalidDataException("Duplicate draft property.");
                RejectDuplicateProperties(property.Value);
            }
        }
        else if (element.ValueKind == JsonValueKind.Array)
            foreach (var item in element.EnumerateArray()) RejectDuplicateProperties(item);
    }

    public static SavedCompositionDraft Import(Stream input)
    {
        try
        {
            using var compressed = new MemoryStream(ReadBounded(input), false);
            using var zip = new ZipArchive(compressed, ZipArchiveMode.Read);
            if (zip.Entries.Count != 1 || zip.Entries[0].FullName != EntryName)
                throw new InvalidDataException("Expected exactly one draft.json entry.");
            var entry = zip.Entries[0];
            if (entry.Length > MaximumBytes || (entry.ExternalAttributes >> 16 & 0xf000) == 0xa000)
                throw new InvalidDataException("Invalid draft entry size or type.");
            using var stream = entry.Open();
            var bytes = ReadBounded(stream);
            if (bytes.LongLength != entry.Length) throw new InvalidDataException("Truncated draft entry.");
            uint crc = uint.MaxValue;
            foreach (var value in bytes)
            {
                crc ^= value;
                for (var bit = 0; bit < 8; bit++) crc = (crc >> 1) ^ ((crc & 1) == 0 ? 0 : 0xedb88320u);
            }
            if (~crc != entry.Crc32) throw new InvalidDataException("Draft entry checksum mismatch.");
            using var json = JsonDocument.Parse(bytes);
            RejectDuplicateProperties(json.RootElement);
            var document = JsonSerializer.Deserialize(bytes, CompositionArchiveJsonContext.Default.CompositionArchiveDocument)
                ?? throw new InvalidDataException("Missing draft document.");
            if (document.Version != 1) throw new InvalidDataException("Unsupported draft archive version.");
            CompositionDraftStore.Validate(new(1, [document.Draft]));
            using var canonical = JsonDocument.Parse(JsonSerializer.SerializeToUtf8Bytes(document, CompositionArchiveJsonContext.Default.CompositionArchiveDocument));
            RequireFields(json.RootElement, canonical.RootElement);
            return document.Draft;
        }
        catch (Exception e) when (e is JsonException or Sloop.Sequencing.EditException or ArgumentException or OverflowException)
        {
            throw new InvalidDataException("Invalid composition draft archive: " + e.Message, e);
        }
    }

    static void RequireFields(JsonElement actual, JsonElement expected)
    {
        if (expected.ValueKind == JsonValueKind.Object)
            foreach (var property in expected.EnumerateObject())
            {
                if (!actual.TryGetProperty(property.Name, out var value)) throw new InvalidDataException("Missing draft field: " + property.Name);
                RequireFields(value, property.Value);
            }
        else if (expected.ValueKind == JsonValueKind.Array)
        {
            var index = 0;
            foreach (var item in expected.EnumerateArray()) RequireFields(actual[index++], item);
        }
    }

    /// <summary>Called only after review approval. Existing entries are never overwritten.</summary>
    public static SavedCompositionDraft Publish(string catalogPath, SavedCompositionDraft imported)
    {
        var entry = imported with { Id = Guid.NewGuid() };
        var catalog = CompositionDraftStore.Load(catalogPath);
        CompositionDraftStore.Save(catalogPath, catalog with { Drafts = catalog.Drafts.Add(entry) });
        return entry;
    }
}
