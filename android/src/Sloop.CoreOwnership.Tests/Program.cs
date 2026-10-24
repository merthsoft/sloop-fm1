using System.Collections.Concurrent;
using Sloop.Protocol;
using Sloop.Workstation;
using Sloop.Android.Services;

int checks=0;
void Check(bool value,string message){if(!value)throw new Exception(message);checks++;}
void Reject(Action action){try{action();}catch(IOException){return;}throw new Exception("Expected send failure");}
async Task Until(Func<bool> condition){using var timeout=new CancellationTokenSource(TimeSpan.FromSeconds(5));while(!condition())await Task.Delay(5,timeout.Token);}

// Throw both before and after receiver acceptance. Cleanup must survive either ambiguity.
foreach(bool after in new[]{false,true})foreach(int failAt in new[]{1,2,3})foreach(bool failOn in new[]{false,true}) {
    var receiver=new NoteReceiver();var live=new LiveNotes(receiver.Send);
    LiveNote[] chord=[new(0,60),new(0,64),new(1,67)];
    if(!failOn)live.Set("finger",chord,100);
    receiver.Fail(failOn,failAt,after);
    Reject(()=>live.Set("finger",failOn?chord:[],100));
    receiver.Recover();
    live.Set("finger",[],100);live.Set("finger",[],100);live.Panic();
    Check(live.Count==0&&receiver.Sounding.Count==0,$"Cleanup after {(failOn?"on":"off")} #{failAt}, after={after}");
}
foreach(bool after in new[]{false,true}) {
    var receiver=new NoteReceiver();var live=new LiveNotes(receiver.Send);
    live.Set("a",[new(0,60),new(0,64)],100);
    live.Set("b",[new(0,60)],80);
    int commonOffs=receiver.Events.Count(b=>b[0]==0x80&&b[1]==60);
    receiver.Fail(false,1,after);
    Reject(()=>live.Set("a",[],100));receiver.Recover();
    live.Set("a",[],100);
    Check(receiver.Sounding.SetEquals([new(0,60)])&&receiver.Events.Count(b=>b[0]==0x80&&b[1]==60)==commonOffs,"Failed release preserves the other owner's shared pitch");
    live.Set("b",[],100);Check(receiver.Sounding.Count==0&&live.Count==0,"Final shared release is balanced");

    // A failed revoice may already have silenced its old pitch; a new owner can restore it.
    live.Set("a",[new(0,60)],100);receiver.Fail(false,1,after);
    Reject(()=>live.Set("a",[new(0,64)],90));receiver.Recover();
    live.Set("b",[new(0,60)],80);
    Check(receiver.Sounding.SetEquals([new(0,60),new(0,64)]),"Revoice reconciles ambiguous off before restoring shared intent");
    live.Panic();Check(receiver.Sounding.Count==0,"Revoice cleanup");

    live.Set("a",[new(0,60)],100);receiver.Fail(false,1,after);live.Panic();
    Check(live.Count==1,"Failed panic retains a cleanup obligation");receiver.Recover();live.Panic();
    Check(live.Count==0&&receiver.Sounding.Count==0,"Repeated panic retries ambiguous off");
    live.Set("a",[new(0,60)],100);receiver.Fail(false,1,after);live.Panic();live.Forget();
    Check(live.Count==0,"Explicit epoch end discards old cleanup obligations");

    receiver=new NoteReceiver();using var block=new PerformancePlayer(receiver.Send);
    block.Hold("latch",[new(0,60),new(0,64)],100,PlayStyle.Block);
    block.Hold("finger",[new(0,60)],80,PlayStyle.Block);receiver.Fail(false,1,after);
    Reject(()=>block.Hold("latch",[new(0,60),new(0,64)],90,PlayStyle.Block,retrigger:true));receiver.Recover();
    block.Hold("latch",[new(0,60),new(0,64)],90,PlayStyle.Block,retrigger:true);
    block.Release("latch");Check(receiver.Sounding.SetEquals([new(0,60)]),"Retry after failed latch attack preserves finger ownership");
    block.Release("finger");Check(!block.Active&&receiver.Sounding.Count==0,"Retried latch/finger cleanup is balanced");
}
// Timed player catches sender errors and its finally path still releases accepted notes.
foreach(bool after in new[]{false,true}) {
    var receiver=new NoteReceiver();using var player=new PerformancePlayer(receiver.Send);
    var failed=new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
    player.Failed+=_=>failed.TrySetResult();receiver.Fail(true,2,after);
    player.Hold("strum",[new(0,60),new(0,64),new(0,67)],100,PlayStyle.StrumUp,strumMs:5);
    await failed.Task.WaitAsync(TimeSpan.FromSeconds(5));await Until(()=>!player.Active);
    Check(receiver.Sounding.Count==0,"Timed job failure finalizer releases partially accepted chord");
}

// This links the production Android adapter, with only Android shell/context types replaced.
using var wire=new GestureWire();using var editor=new EditorClient(wire);
var connection=new Fm1Connection(editor);using var epoch=new CancellationTokenSource();
await connection.InitializeHardwarePerformanceAsync(editor,epoch.Token);
await connection.PressHardwareAsync(1,HardwareGestureKind.Fill);
await Until(()=>wire.Renews>0);
for(int i=0;i<4;i++){await connection.ClearHardwarePerformanceAsync();await connection.InitializeHardwarePerformanceAsync(editor,epoch.Token);}
Check(wire.Discoveries==1,"Song refresh reuses capabilities/session within the same editor epoch");
await connection.PressHardwareAsync(2,HardwareGestureKind.Fill);
Check(wire.PressedTokens.SequenceEqual(new uint[]{1,2}),"Refresh preserves monotonically increasing gesture owner tokens");
var ownedTask=connection.RenewalTask;await connection.CloseHardwarePerformanceAsync();int stopped=wire.Renews;
await Task.Delay(450);Check(ownedTask.IsCompleted&&wire.Renews==stopped,"Close joins the owned timer and stops renewals");

// Close invalidates an outstanding capability discovery before it can publish a session.
using var delayedWire=new GestureWire{DelayDiscovery=true};using var delayedEditor=new EditorClient(delayedWire);
connection.UseEditor(delayedEditor);
var initializing=connection.InitializeHardwarePerformanceAsync(delayedEditor,default);
await delayedWire.DiscoveryEntered.Task.WaitAsync(TimeSpan.FromSeconds(5));
await connection.CloseHardwarePerformanceAsync();delayedWire.CompleteDiscovery();await initializing;
Check(!connection.CanPerformHardware,"Close during discovery prevents stale initialization");

// Reconnect while the old renewal request is in flight: old retirement must not clear the new owner.
using var oldWire=new GestureWire{DelayRenewal=true};using var oldEditor=new EditorClient(oldWire);
connection.UseEditor(oldEditor);await connection.InitializeHardwarePerformanceAsync(oldEditor,default);
await connection.PressHardwareAsync(3,HardwareGestureKind.Fill);
await oldWire.RenewalEntered.Task.WaitAsync(TimeSpan.FromSeconds(5));var retiring=connection.CloseHardwarePerformanceAsync();
Check(!retiring.IsCompleted,"Retirement waits for the in-flight renewal to reconcile");
using var newWire=new GestureWire();using var newEditor=new EditorClient(newWire);
connection.UseEditor(newEditor);var replacementInit=connection.InitializeHardwarePerformanceAsync(newEditor,default);
Check(!replacementInit.IsCompleted,"Reconnect awaits detached prior retirement");
oldWire.CompleteRenewal();await retiring;await replacementInit;
await connection.PressHardwareAsync(4,HardwareGestureKind.Fill);
Check(connection.CanPerformHardware&&newWire.Clears==0,"Old retirement preserves the replacement editor session");
await Until(()=>newWire.Renews>0);await connection.CloseHardwarePerformanceAsync();

// Reopening the same editor while close is pending must not let its late CLEAR erase new gestures.
using var reopeningWire=new GestureWire{DelayRenewal=true};using var reopeningEditor=new EditorClient(reopeningWire);
connection.UseEditor(reopeningEditor);await connection.InitializeHardwarePerformanceAsync(reopeningEditor,default);
await connection.PressHardwareAsync(5,HardwareGestureKind.Fill);await reopeningWire.RenewalEntered.Task.WaitAsync(TimeSpan.FromSeconds(5));
var closing=connection.CloseHardwarePerformanceAsync();var reopening=connection.InitializeHardwarePerformanceAsync(reopeningEditor,default);
Check(!reopening.IsCompleted,"Same-editor reopening waits for prior CLEAR");
reopeningWire.CompleteRenewal();await closing;await reopening;int clearsBeforePress=reopeningWire.Clears;
await connection.PressHardwareAsync(6,HardwareGestureKind.Fill);await Until(()=>reopeningWire.Renews>=2);
Check(reopeningWire.Clears==clearsBeforePress,"Prior close cannot clear a newly pressed replacement gesture");
await connection.CloseHardwarePerformanceAsync();

// Caller cancellation during discovery must not install a renewal loop.
using var cancelWire=new GestureWire{DelayDiscovery=true};using var cancelEditor=new EditorClient(cancelWire);using var cancelled=new CancellationTokenSource();
connection.UseEditor(cancelEditor);var cancelledInit=connection.InitializeHardwarePerformanceAsync(cancelEditor,cancelled.Token);
await cancelWire.DiscoveryEntered.Task.WaitAsync(TimeSpan.FromSeconds(5));cancelled.Cancel();
try{await cancelledInit;throw new Exception("Expected initialization cancellation");}catch(OperationCanceledException){}
cancelWire.CompleteDiscovery();Check(!connection.CanPerformHardware,"Cancelled discovery publishes no session");
await connection.CloseHardwarePerformanceAsync();
Console.WriteLine($"Core ownership: {checks} sender ambiguity, shared-note, timer retirement and connection-epoch checks passed.");

sealed class NoteReceiver
{
    readonly object sync=new();
    public HashSet<LiveNote> Sounding=[];
    public List<byte[]> Events=[];
    bool? failOn;int remaining;bool after;
    public void Fail(bool on,int ordinal,bool afterAcceptance){lock(sync){failOn=on;remaining=ordinal;after=afterAcceptance;}}
    public void Recover(){lock(sync)failOn=null;}
    public void Send(byte[] bytes){lock(sync){bool on=(bytes[0]&0xf0)==0x90;bool fail=failOn==on&&--remaining==0;if(fail)failOn=null;
        if(fail&&!after)throw new IOException("Before acceptance");
        Events.Add(bytes);var note=new LiveNote(bytes[0]&15,bytes[1]);if(on)Sounding.Add(note);else Sounding.Remove(note);
        if(fail)throw new IOException("After acceptance");}}
}
sealed class GestureWire:IMidiTransport,IDisposable
{
    public event Action<ReadOnlyMemory<byte>>? BytesReceived;
    public event Action? Disconnected{add{}remove{}}
    public bool DelayDiscovery,DelayRenewal;
    public int Discoveries,Renews,Clears;
    public ConcurrentQueue<uint> PressedTokens=[];
    public TaskCompletionSource DiscoveryEntered=new(TaskCreationOptions.RunContinuationsAsynchronously),RenewalEntered=new(TaskCreationOptions.RunContinuationsAsynchronously);
    byte[]? delayedRenewal;
    void Reply(byte[] args)=>BytesReceived?.Invoke(EditorCodec.Encode(86,args));
    public void CompleteDiscovery(){DelayDiscovery=false;Reply([0,0,1,7,8,116,3,27]);}
    public void CompleteRenewal(){DelayRenewal=false;Reply(delayedRenewal!);}
    public ValueTask SendAsync(ReadOnlyMemory<byte> bytes,CancellationToken token){token.ThrowIfCancellationRequested();var a=bytes.Span[5..^1].ToArray();byte op=a[0];
        if(op==0){Interlocked.Increment(ref Discoveries);DiscoveryEntered.TrySetResult();if(!DelayDiscovery)CompleteDiscovery();}
        else {byte[] reply=[op,0,..(op==4?new byte[4]:a[1..5])];if(op==1)PressedTokens.Enqueue((uint)(a[1]|a[2]<<7|a[3]<<14|a[4]<<21));
            if(op==4)Interlocked.Increment(ref Clears);
            if(op==3){Interlocked.Increment(ref Renews);delayedRenewal=reply;RenewalEntered.TrySetResult();if(DelayRenewal)return ValueTask.CompletedTask;}
            Reply(reply);}
        return ValueTask.CompletedTask;
    }
    public void Dispose(){}
}
namespace Sloop.Android.Services
{
    public sealed record SnapshotStub(bool Busy,bool IsSimulated,bool IsGenericMidi,DeviceInfo Device);
    public sealed class HandlerStub{public void Post(Action action)=>action();}
    public sealed partial class Fm1Connection
    {
        EditorClient client;readonly HandlerStub handler=new();
        public event Action? Changed;
        public SnapshotStub Snapshot{get;}=new(false,false,false,new("test",1,71,45,64,63,["FM6"],4,15));
        public Fm1Connection(EditorClient editor)=>client=editor;
        public void UseEditor(EditorClient editor)=>client=editor;
        public Task RenewalTask=>hardwareRenewalTask!;
    }
}
