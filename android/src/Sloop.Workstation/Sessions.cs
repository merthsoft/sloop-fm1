using System.IO.Compression;
using System.Security.Cryptography;
using System.Text.Json;

namespace Sloop.Workstation;

public sealed record SessionAsset(string Path, long Length, string Sha256);
public sealed record SessionManifest(int Schema, Guid Id, string Name, DateTimeOffset CreatedUtc,
    SessionAsset[] Assets, Guid[] SceneIds, Guid[] PresetIds);
public sealed record SessionCandidate(SessionManifest Manifest, string Directory);
public sealed record SessionInput(string Path, string Source);
public sealed record PerformancePreset(Guid Id, string Name, DateTimeOffset UpdatedUtc, Dictionary<string,string> Settings);

/// <summary>Immutable generations: only the durable current pointer changes on save/load.</summary>
public sealed class SessionStore
{
    public const int MaximumEntries=512;
    public const long MaximumAssetBytes=256L*1024*1024, MaximumTotalBytes=512L*1024*1024;
    readonly string root;
    readonly object gate=new();
    static readonly JsonSerializerOptions Json=new(){WriteIndented=true};
    public SessionStore(string root){this.root=System.IO.Path.GetFullPath(root);System.IO.Directory.CreateDirectory(this.root);}
    public static void RequirePath(string path)
    {
        if(string.IsNullOrWhiteSpace(path)||path.Length>240||path.Contains('\\')||path.Contains(':')||path.StartsWith('/')||
            path.Split('/').Any(p=>p.Length==0||p is "." or ".."||p.EndsWith('.')||p.EndsWith(' ')||p.Any(c=>c<32||"<>\"|?*".Contains(c))))
            throw new InvalidDataException("Unsafe portable asset path.");
        foreach(var part in path.Split('/')) {
            var stem=part.Split('.')[0].ToUpperInvariant();
            if(stem is "CON" or "PRN" or "AUX" or "NUL"||Enumerable.Range(1,9).Any(i=>stem=="COM"+i||stem=="LPT"+i))throw new InvalidDataException("Reserved asset path.");
        }
    }
    static string Name(string name){name=name.Trim();if(name.Length is <1 or >80||name.Any(char.IsControl))throw new ArgumentException("Use a name of 1–80 characters.");return name;}
    static string Hash(string path){using var s=File.OpenRead(path);return Convert.ToHexString(SHA256.HashData(s));}
    static void Durable(string path,byte[] bytes){var temp=path+"."+Guid.NewGuid().ToString("N")+".tmp";try{using(var s=new FileStream(temp,FileMode.CreateNew,FileAccess.Write,FileShare.None)){s.Write(bytes);s.Flush(true);}File.Move(temp,path,true);}finally{if(File.Exists(temp))File.Delete(temp);}}
    string Location(Guid id)=>System.IO.Path.Combine(root,id.ToString("N"));
    public SessionCandidate Save(string name,IEnumerable<SessionInput> inputs,Guid[]? scenes=null,Guid[]? presets=null,Action<SessionCandidate>? validateContent=null)
    {
        lock(gate){var id=Guid.NewGuid();var stage=System.IO.Path.Combine(root,"stage-"+id.ToString("N"));System.IO.Directory.CreateDirectory(stage);
            try{var assets=new List<SessionAsset>();var seen=new HashSet<string>(StringComparer.OrdinalIgnoreCase);long total=0;
                foreach(var input in inputs){RequirePath(input.Path);if(input.Path.Equals("manifest.json",StringComparison.OrdinalIgnoreCase)||!seen.Add(input.Path)||assets.Count>=MaximumEntries)throw new InvalidDataException("Duplicate or excessive assets.");
                    if((File.GetAttributes(input.Source)&FileAttributes.ReparsePoint)!=0)throw new InvalidDataException("Linked source assets are not supported.");
                    var length=new FileInfo(input.Source).Length;if(length>MaximumAssetBytes||(total+=length)>MaximumTotalBytes)throw new InvalidDataException("Session exceeds size limit.");
                    var dest=System.IO.Path.Combine(stage,input.Path);System.IO.Directory.CreateDirectory(System.IO.Path.GetDirectoryName(dest)!);
                    using(var source=File.OpenRead(input.Source))using(var output=new FileStream(dest,FileMode.CreateNew,FileAccess.Write)){CopyBounded(source,output,length);output.Flush(true);}
                    assets.Add(new(input.Path,length,Hash(dest)));
                }
                var manifest=new SessionManifest(1,id,Name(name),DateTimeOffset.UtcNow,assets.ToArray(),scenes??[],presets??[]);
                Durable(System.IO.Path.Combine(stage,"manifest.json"),JsonSerializer.SerializeToUtf8Bytes(manifest,Json));
                var candidate=Validate(stage);validateContent?.Invoke(candidate);System.IO.Directory.Move(stage,Location(id));return candidate with{Directory=Location(id)};
            }finally{if(System.IO.Directory.Exists(stage))System.IO.Directory.Delete(stage,true);}}
    }
    static void CopyBounded(Stream source,Stream output,long maximum){var buffer=new byte[65536];long size=0;int n;while((n=source.Read(buffer))>0){size+=n;if(size>maximum)throw new InvalidDataException("Asset expanded beyond declared size.");output.Write(buffer,0,n);}if(size!=maximum)throw new InvalidDataException("Truncated asset.");}
    static void CopyArchive(Stream source,Stream output){var buffer=new byte[65536];long size=0;int n;while((n=source.Read(buffer))>0){size+=n;if(size>MaximumTotalBytes+8L*1024*1024)throw new InvalidDataException("Compressed archive exceeds size limit.");output.Write(buffer,0,n);}}
    public SessionCandidate Validate(string directory)
    {
        var path=System.IO.Path.Combine(directory,"manifest.json");if(new FileInfo(path).Length>1024*1024)throw new InvalidDataException("Manifest too large.");
        var m=JsonSerializer.Deserialize<SessionManifest>(File.ReadAllBytes(path))??throw new InvalidDataException("Missing manifest.");
        if(m.Schema!=1||m.Id==Guid.Empty||m.Assets is null||m.Assets.Length>MaximumEntries||m.SceneIds is null||m.PresetIds is null||m.SceneIds.Length>512||m.PresetIds.Length>512)throw new InvalidDataException("Unsupported or invalid session.");Name(m.Name);
        long total=0;var seen=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
        foreach(var a in m.Assets){RequirePath(a.Path);if(a.Path.Equals("manifest.json",StringComparison.OrdinalIgnoreCase)||!seen.Add(a.Path)||a.Length<0||a.Length>MaximumAssetBytes||(total+=a.Length)>MaximumTotalBytes)throw new InvalidDataException("Invalid asset list.");
            var file=System.IO.Path.Combine(directory,a.Path);if(new FileInfo(file).Length!=a.Length||Hash(file)!=a.Sha256)throw new InvalidDataException("Missing or changed asset: "+a.Path);}
        return new(m,directory);
    }
    public SessionCandidate Open(Guid id)=>Validate(Location(id));
    public SessionManifest[] List()=>System.IO.Directory.GetDirectories(root).Where(p=>Guid.TryParseExact(System.IO.Path.GetFileName(p),"N",out _)).Select(p=>{try{return Validate(p).Manifest;}catch{return null;}}).OfType<SessionManifest>().OrderByDescending(m=>m.CreatedUtc).ToArray();
    public SessionCandidate? Current=>File.Exists(System.IO.Path.Combine(root,"current"))?Open(Guid.Parse(File.ReadAllText(System.IO.Path.Combine(root,"current")))):null;
    public void Activate(Guid id){lock(gate){Open(id);var pointer=System.IO.Path.Combine(root,"current");if(File.Exists(pointer))Durable(pointer+".previous",File.ReadAllBytes(pointer));Durable(pointer,System.Text.Encoding.UTF8.GetBytes(id.ToString("N")));}}
    public void Export(Guid id,Stream output){lock(gate){var c=Open(id);using var zip=new ZipArchive(output,ZipArchiveMode.Create,true);foreach(var path in c.Manifest.Assets.Select(a=>a.Path).Prepend("manifest.json")){var entry=zip.CreateEntry(path,CompressionLevel.Fastest);using var target=entry.Open();using var source=File.OpenRead(System.IO.Path.Combine(c.Directory,path));source.CopyTo(target);}}}
    public SessionCandidate Import(Stream input,Action<SessionCandidate>? validateContent=null)
    {
        lock(gate){var stage=System.IO.Path.Combine(root,"stage-"+Guid.NewGuid().ToString("N"));System.IO.Directory.CreateDirectory(stage);
            try{var archivePath=System.IO.Path.Combine(stage,"input.tmp");using(var spool=File.Create(archivePath))CopyArchive(input,spool);
                using(var spool=File.OpenRead(archivePath))using(var zip=new ZipArchive(spool,ZipArchiveMode.Read,true)){if(zip.Entries.Count>MaximumEntries+1)throw new InvalidDataException("Too many archive entries.");long total=0;var seen=new HashSet<string>(StringComparer.OrdinalIgnoreCase);
                foreach(var entry in zip.Entries){RequirePath(entry.FullName);if(!seen.Add(entry.FullName)||entry.Length>MaximumAssetBytes||entry.FullName.Equals("manifest.json",StringComparison.OrdinalIgnoreCase)&&entry.Length>1024*1024||(total+=entry.Length)>MaximumTotalBytes||(entry.ExternalAttributes>>16&0xF000)==0xA000)throw new InvalidDataException("Unsafe archive entry.");
                    var dest=System.IO.Path.Combine(stage,entry.FullName);System.IO.Directory.CreateDirectory(System.IO.Path.GetDirectoryName(dest)!);using var source=entry.Open();using var target=new FileStream(dest,FileMode.CreateNew,FileAccess.Write);CopyBounded(source,target,entry.Length);target.Flush(true);}}
                File.Delete(archivePath);var candidate=Validate(stage);var expected=candidate.Manifest.Assets.Select(a=>a.Path).Append("manifest.json").ToHashSet(StringComparer.OrdinalIgnoreCase);
                if(System.IO.Directory.GetFiles(stage,"*",SearchOption.AllDirectories).Any(p=>!expected.Contains(System.IO.Path.GetRelativePath(stage,p).Replace('\\','/'))))throw new InvalidDataException("Unreferenced archive entries.");
                validateContent?.Invoke(candidate);var m=candidate.Manifest with{Id=Guid.NewGuid()};Durable(System.IO.Path.Combine(stage,"manifest.json"),JsonSerializer.SerializeToUtf8Bytes(m,Json));System.IO.Directory.Move(stage,Location(m.Id));return new(m,Location(m.Id));
            }finally{if(System.IO.Directory.Exists(stage))System.IO.Directory.Delete(stage,true);}}
    }
    public PerformancePreset SavePreset(string name,Dictionary<string,string> settings){if(settings.Count>64||settings.Any(p=>p.Key.Length>80||p.Value.Length>256))throw new ArgumentException("Preset settings too large.");var p=new PerformancePreset(Guid.NewGuid(),Name(name),DateTimeOffset.UtcNow,new(settings));Durable(System.IO.Path.Combine(root,"preset-"+p.Id.ToString("N")+".json"),JsonSerializer.SerializeToUtf8Bytes(p,Json));return p;}
    public PerformancePreset[] Presets()=>System.IO.Directory.GetFiles(root,"preset-*.json").Select(p=>{try{if(new FileInfo(p).Length>65536)return null;return JsonSerializer.Deserialize<PerformancePreset>(File.ReadAllBytes(p));}catch{return null;}}).OfType<PerformancePreset>().OrderBy(p=>p.Name).ToArray();
}

