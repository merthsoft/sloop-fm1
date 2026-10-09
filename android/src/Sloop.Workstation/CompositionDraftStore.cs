using System.Collections.Immutable;
using System.Text.Json;
using System.Text.Json.Serialization;
using Sloop.Sequencing;

namespace Sloop.Workstation;
public sealed record SavedCompositionDraft(Guid Id, string Name, CompositionDraft Draft, ImmutableArray<CompositionPart> Kept);
public sealed record CompositionCatalog(int Version, ImmutableArray<SavedCompositionDraft> Drafts);
[JsonSerializable(typeof(CompositionCatalog))]
internal partial class CompositionJsonContext : JsonSerializerContext;
/// <summary>Device-local named drafts; independent of accepted sequence/session material.</summary>
public static class CompositionDraftStore
{
    public const int MaximumBytes=16*1024*1024;
    public static CompositionCatalog Empty=>new(1,[]);
    public static void Validate(CompositionCatalog catalog)
    {
        if(catalog is null || catalog.Version!=1 || catalog.Drafts.IsDefault || catalog.Drafts.Length>128 || catalog.Drafts.Any(d=>d is null) || catalog.Drafts.Select(d=>d.Id).Distinct().Count()!=catalog.Drafts.Length)throw new InvalidDataException("Invalid composition catalog.");
        foreach(var saved in catalog.Drafts) {
            if(saved.Draft is null || saved.Draft.Intent is null || saved.Draft.Pattern is null || saved.Id==Guid.Empty || string.IsNullOrWhiteSpace(saved.Name) || saved.Name.Length>80 || saved.Kept.IsDefault || saved.Kept.Distinct().Count()!=saved.Kept.Length)throw new InvalidDataException("Invalid saved draft.");
            OfflineComposition.ValidateDraft(saved.Draft);
            if(saved.Kept.Any(p=>!saved.Draft.Intent.Parts.Contains(p)))throw new InvalidDataException("Invalid kept part.");
        }
    }
    public static byte[] Encode(CompositionCatalog catalog)
    {
        Validate(catalog);var bytes=JsonSerializer.SerializeToUtf8Bytes(catalog,CompositionJsonContext.Default.CompositionCatalog);
        if(bytes.Length>MaximumBytes)throw new InvalidDataException("Composition catalog exceeds size limit.");return bytes;
    }
    public static CompositionCatalog Decode(ReadOnlySpan<byte> bytes)
    {
        if(bytes.Length>MaximumBytes)throw new InvalidDataException("Composition catalog exceeds size limit.");
        var catalog=JsonSerializer.Deserialize(bytes,CompositionJsonContext.Default.CompositionCatalog)??throw new InvalidDataException("Missing composition catalog.");Validate(catalog);return catalog;
    }
    public static CompositionCatalog Load(string path)
    {
        if(!File.Exists(path))return Empty;
        using var stream=File.OpenRead(path);
        if(stream.Length>MaximumBytes)throw new InvalidDataException("Composition catalog exceeds size limit.");
        var bytes=new byte[(int)stream.Length];stream.ReadExactly(bytes);return Decode(bytes);
    }
    public static void Save(string path,CompositionCatalog catalog)
    {
        var bytes=Encode(catalog);var destination=Path.GetFullPath(path);Directory.CreateDirectory(Path.GetDirectoryName(destination)!);
        var temporary=destination+"."+Guid.NewGuid().ToString("N")+".tmp";
        try {using(var stream=new FileStream(temporary,FileMode.CreateNew,FileAccess.Write,FileShare.None)){stream.Write(bytes);stream.Flush(true);}File.Move(temporary,destination,true);}
        finally {if(File.Exists(temporary))File.Delete(temporary);}
    }
}
