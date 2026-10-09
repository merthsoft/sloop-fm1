using System.Text;

namespace Sloop.Workstation;

public sealed record WorkspaceGeneration(string Root, string ExpectedPreviousRoot);

/// <summary>Writable workspace generations. Preparing never changes live files; committing only replaces a durable pointer.</summary>
public sealed class WorkspaceGenerationStore
{
    readonly string appRoot;
    readonly string generations;
    static readonly System.Collections.Concurrent.ConcurrentDictionary<string,object> Gates = new(StringComparer.OrdinalIgnoreCase);
    readonly object gate;
    static readonly string[] Folders = ["workspaces", "samples", "settings", "session-settings", "patch-imports", "scenes", "presets"];
    string Pointer => Path.Combine(appRoot, "workspace-current");
    public WorkspaceGenerationStore(string appDirectory)
    {
        appRoot = Path.GetFullPath(appDirectory);
        gate=Gates.GetOrAdd(appRoot,_=>new());
        generations = Path.Combine(appRoot, "workspace-generations");
        Directory.CreateDirectory(generations);
    }
    public string ActiveRoot
    {
        get
        {
            lock(gate)
            {
                var current = Resolve(Pointer);
                if(current is not null) return current;
                var previous = Resolve(Pointer + ".previous");
                if(previous is not null) { WritePointer(Pointer, Token(previous)); return previous; }
                return appRoot;
            }
        }
    }
    string? Resolve(string pointer)
    {
        if(!File.Exists(pointer)) return null;
        var token = File.ReadAllText(pointer);
        if(token == "legacy") return appRoot;
        if(!Guid.TryParseExact(token, "N", out _)) return null;
        var root = Path.Combine(generations, token);
        return File.Exists(Path.Combine(root, ".prepared")) ? root : null;
    }
    string Token(string root) => root == appRoot ? "legacy" : Path.GetFileName(root);
    public WorkspaceGeneration StageCopy(string sourceRoot)
    {
        var expected = ActiveRoot;
        var root = Path.Combine(generations, Guid.NewGuid().ToString("N"));
        Directory.CreateDirectory(root);
        long total = 0; int count = 0;
        foreach(var folder in Folders)
        {
            var source = Path.Combine(sourceRoot, folder);
            if(!Directory.Exists(source)) continue;
            CopyDirectory(source, Path.Combine(root, folder));
        }
        return new(root, expected);
        void CopyDirectory(string source, string target)
        {
            if((File.GetAttributes(source) & FileAttributes.ReparsePoint) != 0) throw new InvalidDataException("Linked workspace directories are not supported.");
            Directory.CreateDirectory(target);
            foreach(var file in Directory.GetFiles(source))
            {
                if(file.EndsWith(".tmp", StringComparison.OrdinalIgnoreCase)) continue;
                if((File.GetAttributes(file) & FileAttributes.ReparsePoint) != 0) throw new InvalidDataException("Linked workspace files are not supported.");
                var name = Path.GetFileName(file); SessionStore.RequirePath(name);
                var size = new FileInfo(file).Length;
                if(++count > SessionStore.MaximumEntries || size > SessionStore.MaximumAssetBytes || (total += size) > SessionStore.MaximumTotalBytes) throw new InvalidDataException("Workspace exceeds session limits.");
                using var input = File.OpenRead(file);
                using var output = new FileStream(Path.Combine(target, name), FileMode.CreateNew, FileAccess.Write);
                var buffer=new byte[65536];int read;long copied=0;while((read=input.Read(buffer))>0){copied+=read;if(copied>size)throw new IOException("Workspace changed while preparing.");output.Write(buffer,0,read);}if(copied!=size)throw new IOException("Workspace changed while preparing.");output.Flush(true);
            }
            foreach(var child in Directory.GetDirectories(source)) { var name = Path.GetFileName(child); SessionStore.RequirePath(name); CopyDirectory(child, Path.Combine(target, name)); }
        }
    }
    public void MarkPrepared(WorkspaceGeneration generation)
    {
        RequireOwned(generation.Root);
        WritePointer(Path.Combine(generation.Root, ".prepared"), "1");
    }
    public void Commit(WorkspaceGeneration generation)
    {
        lock(gate)
        {
            RequireOwned(generation.Root);
            if(!File.Exists(Path.Combine(generation.Root, ".prepared"))) throw new InvalidOperationException("All editors must be prepared before activation.");
            if(ActiveRoot != generation.ExpectedPreviousRoot) throw new InvalidOperationException("Workspace changed while preparing; prepare again.");
            WritePointer(Pointer + ".previous", Token(generation.ExpectedPreviousRoot));
            WritePointer(Pointer, Token(generation.Root));
        }
    }
    public void RestorePrevious()
    {
        lock(gate) { var previous = Resolve(Pointer + ".previous") ?? appRoot; WritePointer(Pointer, Token(previous)); }
    }
    public string RecoverValidRoot(Action<string> validate)
    {
        lock(gate)
        {
            var current=ActiveRoot;
            if(current==appRoot)return current;
            try { validate(current);return current; }
            catch(Exception e) when(e is not OutOfMemoryException)
            {
                var previous=Resolve(Pointer+".previous");
                if(previous is not null&&previous!=current&&previous!=appRoot)
                    try {validate(previous);WritePointer(Pointer,Token(previous));return previous;}catch(Exception error)when(error is not OutOfMemoryException){}
                WritePointer(Pointer,"legacy");return appRoot;
            }
        }
    }
    void RequireOwned(string root)
    {
        if(Path.GetDirectoryName(Path.GetFullPath(root)) != generations || !Guid.TryParseExact(Path.GetFileName(root), "N", out _)) throw new InvalidOperationException("Generation does not belong to this workspace.");
    }
    static void WritePointer(string path, string value)
    {
        var temp = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        using(var stream = new FileStream(temp, FileMode.CreateNew, FileAccess.Write, FileShare.None)) { stream.Write(Encoding.UTF8.GetBytes(value)); stream.Flush(true); }
        try { File.Move(temp, path, true); } finally { if(File.Exists(temp)) File.Delete(temp); }
    }
}
