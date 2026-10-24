using System.Text.Json;

namespace Sloop.Android;

// Only small request IDs go in saved instance state. Files survive activity/process recreation.
internal sealed class UiFileRequests(string directory)
{
    internal sealed record Import(Guid Id,string Workspace,int Track);
    internal sealed record Export(Guid Id,string Name,string Mime);
    string PathFor(Guid id,string suffix)=>Path.Combine(directory,id.ToString("N")+suffix);
    void Store<T>(Guid id,T value)
    {
        Directory.CreateDirectory(directory);
        File.WriteAllText(PathFor(id,".json"),JsonSerializer.Serialize(value));
    }
    T Read<T>(Guid id)=>JsonSerializer.Deserialize<T>(File.ReadAllText(PathFor(id,".json")))
        ??throw new IOException("Picker request expired. Choose Import or Export again.");
    public Import BeginImport(string workspace,int track)
    {
        if(track is <0 or >2)throw new ArgumentOutOfRangeException(nameof(track));
        var request=new Import(Guid.NewGuid(),workspace,track);Store(request.Id,request);return request;
    }
    public Import RequireImport(Guid id,string workspace)
    {
        var request=Read<Import>(id);
        if(request.Id!=id||request.Workspace!=workspace||request.Track is <0 or >2)
            throw new IOException("Import destination changed. Choose Import again.");
        return request;
    }
    public Export StageExport(string name,string mime,Action<Stream> write)
    {
        var request=new Export(Guid.NewGuid(),name,mime);
        try {
            Directory.CreateDirectory(directory);
            using(var output=File.Create(PathFor(request.Id,".data")))write(output);
            Store(request.Id,request);return request;
        }catch{Complete(request.Id);throw;}
    }
    public Export RequireExport(Guid id)
    {
        var request=Read<Export>(id);
        if(request.Id!=id||!File.Exists(PathFor(id,".data")))throw new IOException("Export expired. Choose Export again.");
        return request;
    }
    public void CopyExport(Guid id,Stream destination)
    {
        RequireExport(id);
        using var input=File.OpenRead(PathFor(id,".data"));input.CopyTo(destination);destination.Flush();
    }
    public void Complete(Guid id)
    {
        File.Delete(PathFor(id,".json"));File.Delete(PathFor(id,".data"));
    }
    public void RemoveExpired(DateTime before,params Guid?[] retained)
    {
        if(!Directory.Exists(directory))return;
        foreach(var path in Directory.EnumerateFiles(directory))
            if(Guid.TryParseExact(Path.GetFileNameWithoutExtension(path),"N",out var id)&&
                !retained.Contains(id)&&File.GetLastWriteTimeUtc(path)<before)Complete(id);
    }
}

// Bound to one Activity, not its replaceable view tree. Service notifications may rebuild views.
internal sealed class UiRequestLifetime
{
    long generation;
    bool stopped;
    public long Begin()=>++generation;
    public void Invalidate()=>generation++;
    public void Stop(){stopped=true;Invalidate();}
    public bool IsCurrent(long request,bool usable,object? source,object? current,string root,string currentRoot)=>
        !stopped&&usable&&request==generation&&ReferenceEquals(source,current)&&root==currentRoot;
}

// Install cleanup before onset: even a partially failed press still has a release obligation.
internal sealed class UiBoundedGesture
{
    Action? release;
    long generation;
    public async Task ActivateAsync(Action press,Action release,Func<Task> duration)
    {
        Stop();long activation=++generation;this.release=release;
        try {press();await duration();}
        finally {if(generation==activation)Stop();}
    }
    public void Stop(){generation++;var pending=release;release=null;pending?.Invoke();}
}
