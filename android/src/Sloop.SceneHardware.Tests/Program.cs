using Sloop.Android.Services;
using Sloop.SoundDesign;

int checks=0;
void Check(bool ok,string name) { if(!ok)throw new Exception(name); checks++; }
var sound=new SoundState(FactoryLibrary.Get(0),MacroContext.Neutral);
SceneTrackSound[] baseline=[new(0,sound,3,0),new(1,sound,4,0)];
SceneHardwareTarget[] targets=[new(0,sound with {Macros=new(Feedback:2)}),new(1,sound with {Macros=new(Detune:5)})];
async Task<SceneHardwareResult> Run(int stale=-1,int fail=-1,bool unreadable=false,bool cancel=false,bool selector=false,bool mismatch=false)
{
    var live=baseline.ToDictionary(b=>b.Track); int writes=0;
    if(stale>=0)live[stale]=live[stale] with {Selector=9};
    return await SceneHardwareBatch.Apply(baseline,targets,
        track=> { if(unreadable&&writes>0)throw new IOException("Disconnected"); return Task.FromResult(live[track]); },
        target=> { writes++; live[target.Track]=live[target.Track] with {State=target.State};
            if(selector)live[target.Track]=live[target.Track] with {Preset=10};
            if(mismatch)live[target.Track]=live[target.Track] with {State=sound};
            if(target.Track==fail)throw new IOException("Lost acknowledgment"); return Task.CompletedTask; },
        ()=>{},()=> {if(cancel&&writes>0)throw new OperationCanceledException();});
}
var ok=await Run(); Check(ok.Applied,"Complete readback applies both tracks");
var stale=await Run(stale:1); Check(stale.Tracks.All(t=>t.Outcome==SceneHardwareOutcome.NotApplied),"Last stale baseline prevents every write");
var partial=await Run(fail:0); Check(partial.Tracks[0].Outcome==SceneHardwareOutcome.Applied&&partial.Tracks[1].Outcome==SceneHardwareOutcome.NotApplied,"Lost acknowledgment reconciles actual state and stops remaining writes");
var unknown=await Run(fail:0,unreadable:true); Check(unknown.Tracks.All(t=>t.Outcome==SceneHardwareOutcome.Unknown),"Disconnect never promotes cached baseline to observation");
var cancelled=await Run(cancel:true); Check(cancelled.Tracks[0].Outcome==SceneHardwareOutcome.Applied&&cancelled.Tracks[1].Outcome==SceneHardwareOutcome.NotApplied,"Cancellation still permits read-only reconciliation");
var selector=await Run(selector:true); Check(selector.Tracks[0].Outcome==SceneHardwareOutcome.Partial&&selector.Tracks[1].Outcome==SceneHardwareOutcome.NotApplied,"Preset identity mismatch rejects Applied and stops further writes");
var mismatch=await Run(mismatch:true); Check(mismatch.Tracks[0].Outcome==SceneHardwareOutcome.Partial&&mismatch.Tracks[1].Outcome==SceneHardwareOutcome.NotApplied,"Readback mismatch rejects Applied and stops further writes");
Check(unknown.Tracks.All(t=>t.Observed is null),"Unknown has no observed state");
Console.WriteLine($"Scene hardware: {checks} checks passed.");
