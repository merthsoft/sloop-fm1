using System.Text.Json;
using Sloop.Android.Services;
using Sloop.Workstation;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    WorkspaceGenerationStore Generations=>new(FilesDir!.AbsolutePath);
    public string EffectiveWorkspaceRoot=>Generations.ActiveRoot;
    public Func<string,Task<object?>>? PrepareSessionExtrasAsync {get;set;}
    public Action<object?>? AdoptSessionExtras {get;set;}
    public Func<IEnumerable<SessionInput>>? CaptureSessionExtraAssets {get;set;}
    public Func<Guid[]>? CaptureSessionSceneIds {get;set;}
    static bool performanceSettingsRestored;
    public sealed record PreparedWorkspace(WorkspaceGeneration Generation,EditingWorkspace.Prepared Editing,SampleWorkspace.Prepared Sampling,PerformOptions Options,int Tempo,int[] OutputChannels,int[] EditChannels,object? Extras);
    public void InitializeSessionIntegration()
    {
        AdoptSessionAsync=async candidate=>
        {
            if(samples.Busy||connection.Snapshot.Busy)throw new InvalidOperationException("Wait for sample/device work to finish before loading.");
            StopCompositionAudition();StopPerformance();await connection.StopPlayingAsync();
            SaveSessionSettings();
            var prepared=await PrepareWorkspaceGenerationAsync(candidate.Directory);
            CommitWorkspaceGeneration(prepared);
            ShowWorkspace();
        };
        if(performanceSettingsRestored)return;
        performanceSettingsRestored=true;
        try { var settings=ReadWorkspaceSettings(EffectiveWorkspaceRoot);performOptions=settings.Options;editing.Tempo=settings.Tempo;Array.Copy(settings.OutputChannels,genericSequenceChannels,4);Array.Copy(settings.EditChannels,sequenceEditChannels,4); }
        catch(Exception e){editing.SetStatus("Saved performance settings could not be restored: "+e.Message);}
    }
    public async Task<PreparedWorkspace> PrepareWorkspaceGenerationAsync(string sourceRoot)
    {
        var generation=await Task.Run(()=>Generations.StageCopy(sourceRoot));
        return await PrepareWorkspaceGenerationAsync(generation);
    }
    public async Task<PreparedWorkspace> PrepareWorkspaceGenerationAsync(WorkspaceGeneration generation)
    {
        var edits=await Task.Run(()=>EditingWorkspace.Prepare(generation.Root));
        var sample=await SampleWorkspace.PrepareAsync(generation.Root);
        var settings=ReadWorkspaceSettings(generation.Root);
        var extras=PrepareSessionExtrasAsync is {} prepare?await prepare(generation.Root):null;
        Generations.MarkPrepared(generation);
        return new(generation,edits,sample,settings.Options,settings.Tempo,settings.OutputChannels,settings.EditChannels,extras);
    }
    public void CommitWorkspaceGeneration(PreparedWorkspace prepared)
    {
        StopCompositionAudition();
        if(samples.Busy||connection.Snapshot.Busy||connection.IsRecording||pendingPerformCapture is not null)throw new InvalidOperationException("Finish sample/device/recording work before adopting a workspace.");
        // All deserialization and asset copying completed before this boundary-safe pointer switch.
        Generations.Commit(prepared.Generation);
        editing.Adopt(prepared.Editing);samples.Adopt(prepared.Sampling);performOptions=prepared.Options;editing.Tempo=prepared.Tempo;
        ResetPianoRollForAdoptedWorkspace();
        OnSoundAuditionWorkspaceReplaced();
        Array.Copy(prepared.OutputChannels,genericSequenceChannels,4);Array.Copy(prepared.EditChannels,sequenceEditChannels,4);
        AdoptSessionExtras?.Invoke(prepared.Extras);
        Array.Clear(hardwareSound);hardwareBaseline=null;
    }
    sealed record SavedSettings(PerformOptions Options,int Tempo,int[] OutputChannels,int[] EditChannels);
    SavedSettings ReadWorkspaceSettings(string root)
    {
        var settings=System.IO.Path.Combine(root,"settings","performance.json");
        if(!File.Exists(settings))settings=System.IO.Path.Combine(root,"session-settings","performance.json");
        if(!File.Exists(settings))return new(new(),100,[0,1,2,9],[0,1,2,9]);
        if(new FileInfo(settings).Length>65536)throw new InvalidDataException("Performance settings too large.");
        var map=JsonSerializer.Deserialize<Dictionary<string,string>>(File.ReadAllBytes(settings))??throw new InvalidDataException("Missing settings.");
        int[] Channels(string key) {if(!map.TryGetValue(key,out var value))return [0,1,2,9];var channels=value.Split(',').Select(int.Parse).ToArray();if(channels.Length!=4||channels.Any(c=>c is <0 or >15))throw new InvalidDataException("Invalid channel mapping.");return channels;}
        int tempo=map.TryGetValue("SequenceTempo",out var t)?int.Parse(t):100;if(tempo is <30 or >240)throw new InvalidDataException("Invalid sequence tempo.");
        return new(SessionPresetCodec.Decode(map),tempo,Channels("SequenceOutputChannels"),Channels("SequenceEditChannels"));
    }
    void SaveSessionSettings()
    {
        StopPerformance();
        if(pendingPerformCapture is not null)throw new IOException("Recording could not be saved. Retry Stop / save before saving a session.");
        editing.SaveCurrent();
        var map=SessionPresetCodec.Encode(performOptions);map["SequenceTempo"]=editing.Tempo.ToString(System.Globalization.CultureInfo.InvariantCulture);
        map["SequenceOutputChannels"]=string.Join(',',genericSequenceChannels);map["SequenceEditChannels"]=string.Join(',',sequenceEditChannels);
        WorkspaceFiles.StoreBytes(System.IO.Path.Combine(EffectiveWorkspaceRoot,"settings","performance.json"),JsonSerializer.SerializeToUtf8Bytes(map));
    }
}
