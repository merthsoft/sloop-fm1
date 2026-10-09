using Sloop.SoundDesign;

namespace Sloop.Android.Services;

public enum SoundAuditionStatus { Ready, Original, Candidate, Unknown, Conflict, Kept, Restored }

/// <summary>One reversible RAM preview. Writes are sequential, never atomic.</summary>
public sealed class SoundAudition
{
    readonly Func<Task<SceneHardwareBaseline>> read;
    readonly Func<SceneHardwareBaseline,SoundState,Task<SceneHardwareResult>> write;
    readonly Action stop;
    SceneHardwareBaseline baseline;
    SceneTrackSound expected;
    readonly SceneTrackSound original;
    SoundState? candidate;
    bool busy;
    bool disconnected;
    public string TargetId { get; }
    public long Revision { get; }
    public SoundAuditionStatus Status { get; private set; } = SoundAuditionStatus.Ready;
    public string Message { get; private set; } = "Original hardware baseline acknowledged. Choose local B or original A.";
    public bool Active => Status is not (SoundAuditionStatus.Kept or SoundAuditionStatus.Restored);
    public bool Busy => busy;
    public void NotifyDisconnected()
    {
        if(!Active)return;
        disconnected=true;Status=SoundAuditionStatus.Conflict;
        Message="Connection lost. Original baseline retained; device identity cannot be proven after reconnect. Abandon to preserve current device RAM.";
    }
    void Connected() { if(disconnected)throw new InvalidOperationException(Message); }
    public SoundAudition(SceneHardwareBaseline captured,string targetId,long revision,
        Func<Task<SceneHardwareBaseline>> read,
        Func<SceneHardwareBaseline,SoundState,Task<SceneHardwareResult>> write,Action stop)
    {
        if(captured.Tracks.Length!=1)throw new ArgumentException("Capture exactly one synth track.");
        baseline=captured; original=expected=captured.Tracks[0];
        TargetId=targetId; Revision=revision; this.read=read;this.write=write;this.stop=stop;
    }
    static bool Same(SceneTrackSound a,SceneTrackSound b)=>a.Track==b.Track&&a.Preset==b.Preset&&
        a.Selector==b.Selector&&a.State.Macros==b.State.Macros&&PatchCodec.ContentEquals(a.State.Patch,b.State.Patch);
    void Open() { if(busy||!Active)throw new InvalidOperationException("Audition is busy or closed."); }
    void Target(string id,long revision) { if(id!=TargetId||revision!=Revision)throw new InvalidOperationException("Local target changed. Restore or finish this audition and capture a new baseline."); }
    public Task OriginalAsync()=>Switch(original.State,false);
    public Task CandidateAsync(string id,long revision,SoundState desired)
    {
        Target(id,revision); PatchValidation.Require(desired.Patch);desired.Macros.Validate();
        return Switch(desired,true);
    }
    async Task Switch(SoundState desired,bool isCandidate)
    {
        Open();Connected();
        if(Status is SoundAuditionStatus.Unknown or SoundAuditionStatus.Conflict)throw new InvalidOperationException("Read/reconcile hardware before switching or restoring.");
        busy=true;stop();
        try {
            if(isCandidate)candidate=desired;
            // Mark uncertain before any adapter call; a lost acknowledgement cannot imply success.
            Status=SoundAuditionStatus.Unknown;
            var result=await write(baseline,desired);
            Connected();
            var track=result.Tracks.Single();
            if(track.Outcome!=SceneHardwareOutcome.Applied) {
                Message=track.Error??"Write incomplete. Read/reconcile before another action.";
                return;
            }
            expected=track.Observed??throw new IOException("Missing acknowledged readback.");
            // A fresh baseline carries the adapter's connection token for the next guarded write.
            var fresh=await read();
            Connected();
            if(!ReferenceEquals(fresh.Connection,baseline.Connection)) {NotifyDisconnected();return;}
            if(!Same(expected,fresh.Tracks.Single())) {Status=SoundAuditionStatus.Conflict;Message="Hardware changed after verification; no automatic restore.";return;}
            baseline=fresh;Status=isCandidate?SoundAuditionStatus.Candidate:SoundAuditionStatus.Original;
            Message=isCandidate?"B verified in device RAM. Keep RAM and save locally are separate actions.":"A verified: captured original patch and macros.";
        } catch(Exception e) {if(!disconnected){Status=SoundAuditionStatus.Unknown;Message=e.Message+" Read/reconcile hardware.";}throw;}
        finally {busy=false;}
    }
    public async Task ReconcileAsync()
    {
        Open();Connected();busy=true;stop();Status=SoundAuditionStatus.Unknown;
        try {
            var fresh=await read();var actual=fresh.Tracks.Single();
            Connected();
            if(!ReferenceEquals(fresh.Connection,baseline.Connection)) {Status=SoundAuditionStatus.Conflict;Message="Connection changed. Device identity cannot be proven; preserve RAM and abandon this audition.";return;}
            if(Same(actual,original)) {expected=actual;baseline=fresh;Status=SoundAuditionStatus.Original;Message="Original confirmed by fresh read.";}
            else if(candidate is not null&&Same(actual,original with{State=candidate})) {expected=actual;baseline=fresh;Status=SoundAuditionStatus.Candidate;Message="B confirmed by fresh read.";}
            else {Status=SoundAuditionStatus.Conflict;Message="External edit or partial write detected. Restore/Keep blocked; preserve hardware and abandon this audition.";}
        } catch(Exception e) {if(!disconnected)Message=e.Message+" Hardware remains uncertain.";throw;}
        finally {busy=false;}
    }
    public async Task RestoreAsync() {await OriginalAsync();if(Status==SoundAuditionStatus.Original){Status=SoundAuditionStatus.Restored;Message="Original restored and verified. Local history unchanged.";}}
    public async Task KeepAsync(string id,long revision)
    {
        Open();Target(id,revision);
        if(Status!=SoundAuditionStatus.Candidate)throw new InvalidOperationException("Only verified B can be kept.");
        await ReconcileAsync();
        if(Status!=SoundAuditionStatus.Candidate)throw new InvalidOperationException("Hardware changed; Keep blocked.");
        Status=SoundAuditionStatus.Kept;Message="B retained in device RAM only. Local saving and hardware bank storage are separate.";
    }
}
