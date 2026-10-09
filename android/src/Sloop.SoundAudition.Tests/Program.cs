using Sloop.SoundDesign;
using Sloop.Android.Services;

int checks=0;
void Check(bool condition) {checks++;if(!condition)throw new Exception($"Check {checks} failed");}
async Task Reject(Func<Task> action) {bool rejected=false;try{await action();}catch(InvalidOperationException){rejected=true;}Check(rejected);}
var a=new SoundState(FactoryLibrary.Get(0),MacroContext.Neutral);
var b=new SoundState(FactoryLibrary.Get(1),MacroContext.Neutral);
object epoch=new();var actual=new SceneTrackSound(0,a,1,2);int writes=0,stops=0;bool lost=false,partial=false;
Task<SceneHardwareBaseline> Read()=>Task.FromResult(new SceneHardwareBaseline(epoch,[actual]));
async Task<SceneHardwareResult> Write(SceneHardwareBaseline baseline,SoundState desired) {
    return await SceneHardwareBatch.Apply(baseline.Tracks,[new(0,desired)],_=>Task.FromResult(actual),target=> {
        writes++;actual=actual with{State=partial?new SoundState(target.State.Patch,a.Macros with{Feedback=1}):target.State};
        if(lost)throw new IOException("Lost ACK");return Task.CompletedTask;
    },()=>{});
}
async Task<SoundAudition> Start()=>new(await Read(),"track",7,Read,Write,()=>stops++);
var session=await Start();
await Reject(()=>session.CandidateAsync("other",7,b));Check(writes==0);
await Reject(()=>session.CandidateAsync("track",8,b));Check(writes==0);
await session.CandidateAsync("track",7,b);Check(session.Status==SoundAuditionStatus.Candidate);
await session.OriginalAsync();Check(PatchCodec.ContentEquals(actual.State.Patch,a.Patch));
await session.CandidateAsync("track",7,b);await session.RestoreAsync();Check(session.Status==SoundAuditionStatus.Restored);Check(stops==4);
session=await Start();await session.CandidateAsync("track",7,b);await session.KeepAsync("track",7);Check(session.Status==SoundAuditionStatus.Kept);Check(PatchCodec.ContentEquals(actual.State.Patch,b.Patch));
actual=actual with{State=a};session=await Start();actual=actual with{Selector=3};await session.CandidateAsync("track",7,b);Check(session.Status==SoundAuditionStatus.Unknown);int before=writes;await session.ReconcileAsync();Check(session.Status==SoundAuditionStatus.Conflict);await Reject(session.RestoreAsync);Check(writes==before);
actual=new(0,a,1,2);session=await Start();lost=true;await session.CandidateAsync("track",7,b);Check(session.Status==SoundAuditionStatus.Candidate); // lost ACK but complete subsequent readback is evidence
lost=false;await session.RestoreAsync();Check(session.Status==SoundAuditionStatus.Restored);
session=await Start();partial=true;await session.CandidateAsync("track",7,b);Check(session.Status==SoundAuditionStatus.Unknown);await session.ReconcileAsync();Check(session.Status==SoundAuditionStatus.Conflict);partial=false;
actual=new(0,a,1,2);session=await Start();await session.CandidateAsync("track",7,b);epoch=new();await session.ReconcileAsync();Check(session.Status==SoundAuditionStatus.Conflict);await Reject(()=>session.KeepAsync("track",7));
// An Activity recreation gets the same service and original baseline, with no hardware writes.
actual=new(0,a,1,2);session=await Start();await session.CandidateAsync("track",7,b);
var retained=SoundAuditionRetention.Current;retained.Capture(0,session);before=writes;
var recreated=SoundAuditionRetention.Current;
Check(ReferenceEquals(retained,recreated));Check(ReferenceEquals(recreated[0],session));Check(writes==before);
// Session adoption preserves the recoverable original and exposes the replacement flag.
retained.NotifyWorkspaceReplaced();Check(recreated.WorkspaceReplaced(0));
await recreated[0]!.RestoreAsync();Check(PatchCodec.ContentEquals(actual.State.Patch,a.Patch));
Check(!recreated.WorkspaceReplaced(0));
session=await Start();retained.Capture(0,session);retained.NotifyDisconnected();before=writes;
Check(ReferenceEquals(retained[0],session));Check(session.Status==SoundAuditionStatus.Conflict);
await Reject(session.OriginalAsync);await Reject(session.ReconcileAsync);Check(writes==before);
retained.Abandon(0);Check(retained[0] is null);Check(writes==before);
// A disconnect during an in-flight write cannot be overwritten by a later successful readback.
actual=new(0,a,1,2);
var gate=new TaskCompletionSource<SceneHardwareResult>();
session=new(await Read(),"track",7,Read,(_,_)=>gate.Task,()=>stops++);
retained.Capture(0,session);var pending=session.CandidateAsync("track",7,b);
Check(session.Busy);bool abandonRejected=false;try{retained.Abandon(0);}catch(InvalidOperationException){abandonRejected=true;}Check(abandonRejected);
retained.NotifyDisconnected();actual=actual with{State=b};
gate.SetResult(new([new(0,SceneHardwareOutcome.Applied,actual,null)]));await Reject(()=>pending);
Check(session.Status==SoundAuditionStatus.Conflict);Check(!session.Busy);Check(ReferenceEquals(retained[0],session));
retained.Abandon(0);Check(PatchCodec.ContentEquals(actual.State.Patch,b.Patch));
Console.WriteLine($"Sound audition: {checks} focused checks passed.");
