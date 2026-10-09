using Sloop.Workstation;
using Sloop.Core.Sampling;

namespace Sloop.Android.Services;

/// <summary>App-private snapshot adapter. The shell owns atomic adoption of validated candidates.</summary>
public sealed class SessionWorkspace(string appDirectory,string? workspaceRoot=null)
{
    static readonly System.Collections.Concurrent.ConcurrentDictionary<string,bool> StartupChecked=[];
    public static void RecoverStartup(string appDirectory)
    {
        lock(StartupChecked)
        {
            if(StartupChecked.ContainsKey(appDirectory))return;
            new WorkspaceGenerationStore(appDirectory).RecoverValidRoot(root=>
            {
                EditingWorkspace.Prepare(root);
                SampleWorkspace.PrepareAsync(root,false).GetAwaiter().GetResult();
                var settings=System.IO.Path.Combine(root,"settings","performance.json");
                if(File.Exists(settings)){if(new FileInfo(settings).Length>65536)throw new InvalidDataException("Performance settings too large.");var map=System.Text.Json.JsonSerializer.Deserialize<Dictionary<string,string>>(File.ReadAllBytes(settings))??throw new InvalidDataException("Missing settings.");ValidateSettings(map);}
                var scenes=System.IO.Path.Combine(root,"scenes","catalog.json");if(File.Exists(scenes))Sloop.Scenes.SceneStore.Load(scenes);
            });
            StartupChecked[appDirectory]=true;
        }
    }
    static void ValidateSettings(Dictionary<string,string> map)
    {
        SessionPresetCodec.Decode(map);
        foreach(var key in new[]{"SequenceOutputChannels","SequenceEditChannels"})if(map.TryGetValue(key,out var value)){var channels=value.Split(',').Select(int.Parse).ToArray();if(channels.Length!=4||channels.Any(c=>c is <0 or >15))throw new InvalidDataException("Invalid channel mapping.");}
        if(map.TryGetValue("SequenceTempo",out var tempo)&&int.Parse(tempo) is <30 or >240)throw new InvalidDataException("Invalid sequence tempo.");
    }
    string WorkspaceRoot=>workspaceRoot??new WorkspaceGenerationStore(appDirectory).ActiveRoot;
    public SessionStore Store {get;}=new(System.IO.Path.Combine(appDirectory,"sessions"));
    public PerformancePreset[] Presets()
    {
        var imported=System.IO.Path.Combine(WorkspaceRoot,"presets");
        return Store.Presets().Concat(Directory.Exists(imported)?Directory.GetFiles(imported,"preset-*.json").Select(path=>System.Text.Json.JsonSerializer.Deserialize<PerformancePreset>(File.ReadAllBytes(path))??throw new InvalidDataException("Missing preset.")):[]).DistinctBy(p=>p.Id).OrderBy(p=>p.Name).ToArray();
    }
    public SessionCandidate Save(string name,Guid[]? scenes=null,Guid[]? presets=null,IEnumerable<SessionInput>? extraAssets=null)
    {
        var inputs=new List<SessionInput>();
        if(extraAssets is not null)inputs.AddRange(extraAssets);
        var patches=System.IO.Path.Combine(WorkspaceRoot,"patch-imports");
        if(Directory.Exists(patches))foreach(var file in Directory.GetFiles(patches,"*.syx"))inputs.Add(new("patch-imports/"+System.IO.Path.GetFileName(file),file));
        var settings=System.IO.Path.Combine(WorkspaceRoot,"settings");
        if(Directory.Exists(settings))foreach(var file in Directory.GetFiles(settings).Where(p=>!p.EndsWith(".tmp")))inputs.Add(new("settings/"+System.IO.Path.GetFileName(file),file));
        foreach(var preset in Presets()){
            var file=System.IO.Path.Combine(appDirectory,"sessions","preset-"+preset.Id.ToString("N")+".json");
            if(!File.Exists(file))file=System.IO.Path.Combine(WorkspaceRoot,"presets","preset-"+preset.Id.ToString("N")+".json");
            inputs.Add(new("presets/"+System.IO.Path.GetFileName(file),file));
        }
        var workspace=System.IO.Path.Combine(WorkspaceRoot,"workspaces");
        if(Directory.Exists(workspace))foreach(var file in Directory.GetFiles(workspace).Where(p=>!p.EndsWith(".tmp")))inputs.Add(new("workspaces/"+System.IO.Path.GetFileName(file),file));
        var sampleRoot=System.IO.Path.Combine(WorkspaceRoot,"samples");var pointer=System.IO.Path.Combine(sampleRoot,"current.txt");
        if(File.Exists(pointer)){
            var nameOfSample=File.ReadAllText(pointer);SessionStore.RequirePath(nameOfSample);
            if(nameOfSample.Contains('/'))throw new InvalidDataException("Invalid current sample pointer.");
            inputs.Add(new("samples/current.txt",pointer));
            foreach(var suffix in new[]{"",".edits",".kitsettings",".chopaudio",".fm1"}){var file=System.IO.Path.Combine(sampleRoot,nameOfSample+suffix);if(File.Exists(file))inputs.Add(new("samples/"+nameOfSample+suffix,file));else if(suffix=="")throw new InvalidDataException("Referenced original sample missing.");}
        }
        var result=Store.Save(name,inputs,scenes,presets,ValidateContent);return result;
    }
    public SessionCandidate Prepare(Guid id){var candidate=Store.Open(id);ValidateContent(candidate);return candidate;}
    public SessionCandidate Import(Stream input){var candidate=Store.Import(input,ValidateContent);return candidate;}
    public static void ValidateContent(SessionCandidate candidate)
    {
        var catalogPath=System.IO.Path.Combine(candidate.Directory,"scenes","catalog.json");
        var catalog=File.Exists(catalogPath)?Sloop.Scenes.SceneStore.Load(catalogPath):null;
        if(candidate.Manifest.SceneIds.Any(id=>catalog is null||!catalog.Scenes.Any(s=>s.Id==id)))
            throw new InvalidDataException("Session references a missing scene.");
        var presetIds=new HashSet<Guid>();
        foreach(var a in candidate.Manifest.Assets){var path=System.IO.Path.Combine(candidate.Directory,a.Path);
            if(a.Path.StartsWith("workspaces/sound-")&&a.Path.EndsWith(".sloop"))WorkspaceFiles.LoadSound(path,"session/"+a.Path);
            else if(a.Path=="workspaces/pattern.sloop")WorkspaceFiles.LoadPattern(path);
            else if(a.Path=="workspaces/hardware.sloop")WorkspaceFiles.LoadHardware(path);
            else if(a.Path=="settings/performance.json"){
                if(a.Length>65536)throw new InvalidDataException("Performance settings too large.");
                ValidateSettings(System.Text.Json.JsonSerializer.Deserialize<Dictionary<string,string>>(File.ReadAllBytes(path))??throw new InvalidDataException("Missing performance settings."));
            }
            else if(a.Path.StartsWith("presets/")){
                if(a.Length>65536)throw new InvalidDataException("Preset too large.");
                var preset=System.Text.Json.JsonSerializer.Deserialize<PerformancePreset>(File.ReadAllBytes(path))??throw new InvalidDataException("Missing preset.");SessionPresetCodec.Decode(preset.Settings);presetIds.Add(preset.Id);
            }
        }
        if(candidate.Manifest.PresetIds.Any(id=>!presetIds.Contains(id)))throw new InvalidDataException("Session references a missing preset.");
        var pointer=System.IO.Path.Combine(candidate.Directory,"samples/current.txt");
        if(File.Exists(pointer)){
            var name=File.ReadAllText(pointer);SessionStore.RequirePath(name);if(name.Contains('/'))throw new InvalidDataException("Invalid sample pointer.");
            var source=PcmWave.Open(System.IO.Path.Combine(candidate.Directory,"samples",name));
            ChopAudioSettings.ValidateFile(source);
            var edits=source.Path+".edits";if(File.Exists(edits))SampleDocument.Restore(source,edits);
        }
    }
}

