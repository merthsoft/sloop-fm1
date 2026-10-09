using Sloop.Sequencing;
using Sloop.Workstation;
using System.Collections.Immutable;

int passed=0;
void Check(bool value,string name){if(!value)throw new Exception(name);passed++;Console.WriteLine("PASS "+name);}
AppPattern Pattern(params AppNote[] notes)=>new(Guid.NewGuid(),Guid.NewGuid(),96,new(384),notes.ToImmutableArray());
byte[] On(int pitch=60,int channel=0,int velocity=100)=>[(byte)(0x90+channel),(byte)pitch,(byte)velocity];
byte[] Off(int pitch=60,int channel=0)=>[(byte)(0x80+channel),(byte)pitch,0];
var p=Pattern();
var capture=new PerformanceCapture(p,CaptureMode.Overdub,120);
capture.ObserveAt(On(),360);capture.ObserveAt(Off(),408);
var proposal=capture.FinishAt(410);var after=(AppPattern)proposal.After;
Check(after.Notes.Length==2&&after.Notes[0].Start.Value==360&&after.Notes[0].Duration.Value==24&&after.Notes[1].Start.Value==0&&after.Notes[1].Duration.Value==24,"loop wrap splits exact duration");
var history=new EditHistory(p);history.Apply(proposal);history.Undo();Check(((AppPattern)history.Current).Notes.Length==0,"recording undo is atomic");history.Redo();Check(((AppPattern)history.Current).Notes.Length==2,"recording redo");
capture=new(p,CaptureMode.Overdub,120);capture.ObserveAt(On(),-20);capture.ObserveAt(Off(),48);after=(AppPattern)capture.FinishAt(60).After;
Check(after.Notes.Single().Start.Value==0&&after.Notes.Single().Duration.Value==48,"count-in held note carries at tick zero");
capture=new(p,CaptureMode.Overdub,120);capture.ObserveAt(On(),-30);capture.ObserveAt(Off(),-10);Check(capture.FinishAt(40).Changes.Length==0,"count-in only notes ignored");
capture=new(p,CaptureMode.Overdub,120);capture.ObserveAt(On(),5);capture.ObserveAt(On(),7);capture.ObserveAt(Off(),15);after=(AppPattern)capture.FinishAt(30).After;Check(after.Notes.Length==1&&after.Notes[0].Duration.Value==10,"duplicate MIDI onset retains original duration");
var existing=new AppNote(Guid.NewGuid(),p.Id,new(0),new(48),50,90,1);
p=p with{Notes=[existing,new(Guid.NewGuid(),p.Id,new(0),new(48),51,90,0)]};
capture=new(p,CaptureMode.ReplaceChannels,120);capture.ObserveAt(On(),10);capture.ObserveAt(Off(),40);after=(AppPattern)capture.FinishAt(50).After;
Check(after.Notes.Length==2&&after.Notes.Contains(existing)&&after.Notes.All(n=>n.Pitch!=51),"replace touched channels preserves other channels and IDs");
capture=new(p,CaptureMode.Overdub,120);capture.ObserveAt(On(),300);after=(AppPattern)capture.FinishAt(2000).After;Check(after.Notes.Skip(2).Sum(n=>n.Duration.Value)==384,"long held note bounded to one phrase");
capture=new(p,CaptureMode.Overdub,120);capture.ObserveAt(On(64,9,70),20);capture.ObserveAt([(byte)0x99,64,0],20);after=(AppPattern)capture.FinishAt(30).After;Check(after.Notes.Last().Channel==9&&after.Notes.Last().Velocity==70&&after.Notes.Last().Duration.Value==1,"zero velocity MIDI off and minimum gate");
var clock=new MusicalClock(960,120);await Task.Delay(25);double before=clock.Position;clock.SetTempo(240);double immediately=clock.Position;Check(immediately>=before&&immediately-before<30,"tempo re-anchor preserves position");
var ticks=new List<TransportTick>();var midi=new List<byte[]>();var loop=new PatternLoop();using var cts=new CancellationTokenSource(TimeSpan.FromSeconds(3));
p=Pattern() with{Length=new(96),Notes=[new(Guid.NewGuid(),Guid.NewGuid(),new(0),new(96),60,100,0),new(Guid.NewGuid(),Guid.NewGuid(),new(24),new(48),60,90,0)]};
loop.Tick+=tick=>{ticks.Add(tick);if(tick.Loop==2)cts.Cancel();};
try{await loop.RunAsync(p,240,b=>midi.Add(b),cts.Token);}catch(OperationCanceledException)when(cts.IsCancellationRequested){}
Check(ticks.Count(t=>t.IsLoop)==3&&ticks.Select(t=>t.AbsoluteTick).SequenceEqual(ticks.Select(t=>t.AbsoluteTick).Order()),"continuous loop absolute boundary contract");
Check(midi.Count(b=>(b[0]&0xf0)==0x90)==2&&midi.Count(b=>(b[0]&0xf0)==0x80)==2,"overlapping same pitch ownership and end-before-start at wrap");
Check(loop.Clock is null,"cancellation clears clock");
loop=new PatternLoop();midi.Clear();using var stop=new CancellationTokenSource();loop.Tick+=tick=>{if(tick.AbsoluteTick==24)stop.Cancel();};
try{await loop.RunAsync(p,240,b=>midi.Add(b),stop.Token);}catch(OperationCanceledException){}
Check(midi.Count==2&&(midi.Last()[0]&0xf0)==0x80,"stop releases held notes before returning");
var emittedCapture=new PerformanceCapture(Pattern(),CaptureMode.Overdub,240);
using(var performer=new PerformancePlayer(emittedCapture.Observe)) {
    performer.Hold("arp",[new(0,60),new(0,64)],100,PlayStyle.ArpUp,240,4);
    await Task.Delay(190);performer.Panic();
}
var emitted=(AppPattern)emittedCapture.Finish().After;
Check(emitted.Notes.Length>=2&&emitted.Notes.Select(n=>n.Pitch).Distinct().Count()==2&&emitted.Notes.All(n=>n.Duration.Value>0&&n.Duration.Value<24),"capture actual timed arp gates and panic cleanup");
emittedCapture=new(Pattern(),CaptureMode.Overdub,240);
using(var performer=new PerformancePlayer(emittedCapture.Observe)) {
    performer.Hold("one",[new(0,60)],100,PlayStyle.Block);
    await Task.Delay(20);performer.Hold("two",[new(0,60)],80,PlayStyle.Block);
    await Task.Delay(20);performer.Release("one");await Task.Delay(20);performer.Release("two");
}
Check(((AppPattern)emittedCapture.Finish().After).Notes.Length==1,"capture retains shared performance ownership");
var attacks=new List<byte[]>();
using(var performer=new PerformancePlayer(attacks.Add)) {
    LiveNote[] chord=[new(0,60),new(0,64),new(0,67)];
    performer.Hold("latch",chord,100,PlayStyle.Block,bass:new(1,48));
    attacks.Clear();
    performer.Hold("latch",chord,82,PlayStyle.Block,bass:new(1,48),retrigger:true);
    Check(attacks.Count==8&&attacks.Take(4).All(b=>(b[0]&0xf0)==0x80)&&
        attacks.Skip(4).All(b=>(b[0]&0xf0)==0x90&&b[2]==82)&&
        attacks.Take(4).Select(b=>(b[0]&15,b[1])).ToHashSet().SetEquals(attacks.Skip(4).Select(b=>(b[0]&15,b[1]))),
        "same latched chord and bass release before fresh attacks at requested velocity");
    attacks.Clear();
    performer.Hold("latch",[new(0,60),new(0,65),new(0,67)],82,PlayStyle.Block,bass:new(1,48));
    Check(attacks.Count==2&&attacks[0].SequenceEqual(Off(64))&&attacks[1].SequenceEqual(On(65,0,82)),
        "modifier revoicing preserves common chord and bass notes");
    performer.Hold("finger",[new(0,60)],100,PlayStyle.Block);
    attacks.Clear();
    performer.Hold("latch",chord,90,PlayStyle.Block,bass:new(1,48),retrigger:true);
    Check(attacks.Count==6&&attacks.All(b=>b[1]!=60)&&attacks.Take(3).All(b=>(b[0]&0xf0)==0x80)&&attacks.Skip(3).All(b=>(b[0]&0xf0)==0x90),
        "latched attack respects another owner's shared note");
    attacks.Clear();performer.Release("latch");
    Check(attacks.Count==3&&performer.Active,"releasing retriggered latch keeps shared finger alive");
    performer.Release("finger");
    Check(attacks.Count==4&&attacks.Last().SequenceEqual(Off())&&!performer.Active,"final shared release leaves no stuck notes");
}
loop=new PatternLoop();using var fractionalStop=new CancellationTokenSource();bool beatSeen=false;
loop.Tick+=tick=>{if(tick.AbsoluteTick==95){beatSeen=tick.IsBeat;fractionalStop.Cancel();}};
try{await loop.RunAsync(Pattern() with{TicksPerQuarter=95,Length=new(190)},240,_=>{},fractionalStop.Token);}catch(OperationCanceledException){}
Check(beatSeen,"boundary contract preserves beats for non-divisible PPQ");
loop=new PatternLoop();using var sceneStop=new CancellationTokenSource();bool queuedSeen=false;Guid epoch=Guid.Empty;
loop.Tick+=tick=>{if(tick.AbsoluteTick==0){epoch=tick.TransportEpoch;loop.ScheduleBoundary(85);}if(tick.AbsoluteTick==85){queuedSeen=tick.TransportEpoch==epoch;sceneStop.Cancel();}};
try{await loop.RunAsync(Pattern() with{Length=new(96)},240,_=>{},sceneStop.Token);}catch(OperationCanceledException){}
Check(queuedSeen,"scene due tick inserted after final regular subdivision with stable epoch");
// Hosts reserve identity before handing playback to a worker; repeated direct runs still get fresh epochs.
using var preCancelled=new CancellationTokenSource();preCancelled.Cancel();
var reservedLoop=new PatternLoop();var reservedIdentity=reservedLoop.TransportEpoch;
try{await reservedLoop.RunAsync(Pattern(),120,_=>throw new Exception("Cancelled run sent MIDI"),preCancelled.Token,reservedIdentity);}catch(OperationCanceledException){}
Check(reservedIdentity!=Guid.Empty && reservedLoop.TransportEpoch==reservedIdentity && reservedLoop.Clock is null,"reserved worker identity survives cancellation before first tick");
try{await reservedLoop.RunAsync(Pattern(),120,_=>{},preCancelled.Token);}catch(OperationCanceledException){}
Check(reservedLoop.TransportEpoch!=reservedIdentity,"reusing a loop without a reserved epoch creates a fresh transport identity");
bool emptyEpochRejected=false;
try{await reservedLoop.RunAsync(Pattern(),120,_=>{},preCancelled.Token,Guid.Empty);}catch(ArgumentException){emptyEpochRejected=true;}
Check(emptyEpochRejected && reservedLoop.Clock is null,"empty transport identity rejects before starting playback");
Console.WriteLine($"{passed} transport checks passed.");


