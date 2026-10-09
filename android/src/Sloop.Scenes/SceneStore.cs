using System.Collections.Immutable;
using System.Text.Json;
using System.Text.Json.Serialization;

namespace Sloop.Scenes;
public sealed record SceneCatalog(int Version, ImmutableArray<SceneSnapshot> Scenes,
    ImmutableArray<Arrangement> Arrangements, ImmutableArray<SceneAutomation> Automation)
{
    public void Validate()
    {
        if (Version != 1 || Scenes.IsDefault || Arrangements.IsDefault || Automation.IsDefault ||
            Scenes.Length > 1024 || Arrangements.Length > 1024 || Automation.Length > 8192)
            throw new ArgumentException("Unsupported or oversized scene catalog.");
        var scenes = Scenes.ToDictionary(s => s.Id);
        foreach (var s in Scenes) s.Validate();
        if (Arrangements.Select(a => a.Id).Distinct().Count() != Arrangements.Length)
            throw new ArgumentException("Duplicate arrangement identity.");
        foreach (var a in Arrangements) a.Validate(scenes);
        if (Automation.Select(a => (a.SceneId, a.Lane.Id)).Distinct().Count() != Automation.Length)
            throw new ArgumentException("Duplicate automation identity.");
        foreach (var a in Automation)
        {
            if (!scenes.TryGetValue(a.SceneId, out var scene) || scene.AppPattern is null)
                throw new ArgumentException("Automation requires an app-pattern scene.");
            a.Lane.Validate(scene.AppPattern.Length);
        }
    }
}
public sealed record SceneAutomation(Guid SceneId, AutomationLane Lane);
[JsonSerializable(typeof(SceneCatalog))]
internal partial class SceneJsonContext : JsonSerializerContext;
public static class SceneStore
{
    public const int MaximumBytes = 16 * 1024 * 1024;
    public static byte[] Encode(SceneCatalog catalog)
    {
        catalog.Validate();
        var bytes = JsonSerializer.SerializeToUtf8Bytes(catalog, SceneJsonContext.Default.SceneCatalog);
        if (bytes.Length > MaximumBytes) throw new InvalidDataException("Scene catalog exceeds size limit.");
        return bytes;
    }
    public static SceneCatalog Decode(ReadOnlySpan<byte> bytes)
    {
        if (bytes.Length > MaximumBytes) throw new InvalidDataException("Scene catalog exceeds size limit.");
        var catalog = JsonSerializer.Deserialize(bytes, SceneJsonContext.Default.SceneCatalog)
            ?? throw new InvalidDataException("Missing scene catalog.");
        catalog.Validate(); return catalog;
    }
    public static void Save(string path, SceneCatalog catalog)
    {
        var bytes = Encode(catalog);
        var destination = Path.GetFullPath(path);
        Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
        var temporary = destination + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            using (var stream = new FileStream(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None))
            { stream.Write(bytes); stream.Flush(true); }
            File.Move(temporary, destination, true);
        }
        finally { if (File.Exists(temporary)) File.Delete(temporary); }
    }
    public static SceneCatalog Load(string path)
    {
        using var stream = File.OpenRead(path);
        if (stream.Length > MaximumBytes) throw new InvalidDataException("Scene catalog exceeds size limit.");
        using var buffer = new MemoryStream();
        var block = new byte[8192]; int count;
        while ((count = stream.Read(block)) != 0)
        { if (buffer.Length + count > MaximumBytes) throw new InvalidDataException("Scene catalog exceeds size limit."); buffer.Write(block, 0, count); }
        return Decode(buffer.ToArray());
    }
}
