using Sloop.Workstation;
using Sloop.Sequencing;
using Sloop.Scenes;

int checks=0;
void Check(bool condition,string message){if(!condition)throw new Exception(message);checks++;}
AppPattern Pattern(int pitch,long length=96)=>new(Guid.NewGuid(),Guid.NewGuid(),96,new(length),[new(Guid.NewGuid(),Guid.NewGuid(),new(0),new(length),pitch,100,0)]);
var outgoing=Pattern(60) with{Notes=[new(Guid.NewGuid(),Guid.NewGuid(),new(0),new(96),60,100,0),new(Guid.NewGuid(),Guid.NewGuid(),new(60),new(24),61,100,0)]};
var incoming=Pattern(72,144);
var scene=SceneEditing.Capture("Next",180,[],incoming);
var scheduler=new SceneScheduler(Guid.NewGuid());
var loop=new PatternLoop();var events=new List<(long At,byte[] Bytes)>();long now=0;var epochs=new HashSet<Guid>();
using var stop=new CancellationTokenSource(TimeSpan.FromSeconds(4));
loop.Tick+=tick=>{
    now=tick.AbsoluteTick;epochs.Add(tick.TransportEpoch);
    var position=new MusicalPosition(tick.TransportEpoch,new(tick.AbsoluteTick),tick.TicksPerQuarter,4,new(tick.PatternLength),new(tick.PhraseOrigin));
    if(tick.AbsoluteTick==0&&scheduler.Pending is null){var p=scheduler.Prepare(scene,scheduler.LiveRevision,SceneRestoreMode.AppOnly);var queue=scheduler.Queue(p.Token,position,SceneBoundary.Beat);loop.ScheduleBoundary(queue.Due.Value);}
    if(tick.AbsoluteTick==96&&scheduler.Pending is not null){var commit=scheduler.Commit(position)!;loop.ReplaceAtBoundary(commit.Scene.AppPattern!,commit.Scene.Tempo);scheduler.Reconcile(new(commit.Token,RestoreOutcome.Applied,"app replacement"));}
    if(tick.AbsoluteTick>=240)stop.Cancel();
};
try{await loop.RunAsync(outgoing,240,b=>events.Add((now,b)),stop.Token);}catch(OperationCanceledException)when(stop.IsCancellationRequested){}
Check(events.Count(e=>(e.Bytes[0]&0xf0)==0x90&&e.Bytes[1]==60)==1,"Old phrase does not start again at scene boundary");
int oldOff=events.FindLastIndex(e=>e.Bytes[1]==60&&(e.Bytes[0]&0xf0)==0x80);
int newOn=events.FindIndex(e=>e.Bytes[1]==72&&(e.Bytes[0]&0xf0)==0x90);
Check(oldOff>=0&&newOn>oldOff&&events[newOn].At==96,"Outgoing owned notes release before incoming scene onset");
Check(events.Count(e=>(e.Bytes[0]&0xf0)==0x90&&e.Bytes[1]==61)==1,"Unsent old events discarded after switch");
Check(epochs.Count==1&&scheduler.AwaitingReconciliation is null,"Transport epoch remains stable through reconciled scene switch");
Check(events.Last().Bytes[1]==72&&(events.Last().Bytes[0]&0xf0)==0x80,"Stopping cleans up incoming held note");
Check(loop.Clock is null,"Clock disposed on stop");

// Switch mid-phrase; the next phrase boundary is relative to the new origin.
scheduler=new(Guid.NewGuid());var prepared=scheduler.Prepare(scene,scheduler.LiveRevision,SceneRestoreMode.AppOnly);
var shifted=new MusicalPosition(Guid.NewGuid(),new(150),96,4,new(144),new(96));
Check(scheduler.Queue(prepared.Token,shifted,SceneBoundary.Phrase).Due.Value==240,"Phrase boundary uses scene start origin");
Check(scheduler.Commit(shifted with{Position=new(240)}) is not null,"Shifted phrase commits exactly at due");

loop=new();events.Clear();now=0;bool switched=false;using var midStop=new CancellationTokenSource(TimeSpan.FromSeconds(4));
loop.Tick+=tick=>{now=tick.AbsoluteTick;if(now==0)loop.ScheduleBoundary(48);if(now==48&&!switched){switched=true;loop.ReplaceAtBoundary(incoming,240);}if(now>=192)midStop.Cancel();};
try{await loop.RunAsync(outgoing,240,b=>events.Add((now,b)),midStop.Token);}catch(OperationCanceledException)when(midStop.IsCancellationRequested){}
Check(events.All(e=>e.Bytes[1]!=61),"Mid-phrase transition suppresses later outgoing note");
Check(events.Any(e=>e.At==48&&e.Bytes[1]==72&&(e.Bytes[0]&0xf0)==0x90),"New phrase starts at inserted mid-phrase boundary");
bool rejected=false;try{loop.ReplaceAtBoundary(incoming,240);}catch(InvalidOperationException){rejected=true;}
Check(rejected,"Replacement outside boundary rejected");
Console.WriteLine($"Integration: {checks} scene/transport checks passed.");
