using Sloop.Workstation;
using Sloop.Sequencing;
namespace Sloop.Android.Services;
public sealed partial class Fm1Connection
{
    PerformanceCapture? capture;
    bool captureSenderInstalled;
    public bool IsRecording=>capture is not null;
    public bool IsCountingIn=>capture?.CountingIn==true;
    public void EnablePerformanceCapture()
    {
        if(captureSenderInstalled)return;
        if(performer?.Active==true)throw new IOException("Release notes before enabling recording.");
        performer?.Dispose();
        performer=new(bytes=>{SendPerformance(bytes);capture?.Observe(bytes);});
        captureSenderInstalled=true;
    }
    public void StartCapture(AppPattern pattern,CaptureMode mode,int tempo,int countInBars)
    {
        if(IsRecording||IsPlaying||Performer.Active||Snapshot.Busy)throw new IOException("Stop playback and release notes before recording.");
        PatternValidation.Validate(pattern);
        EnablePerformanceCapture();
        capture=new(pattern,mode,tempo,countInBars);
    }
    public EditProposal? FinishCapture()
    {
        ReleasePerformance();var value=capture;capture=null;return value?.Finish();
    }
    public void SetTransportTempo(int tempo) { loopPlayer?.Clock?.SetTempo(tempo);capture?.SetTempo(tempo); }
    PatternLoop? loopPlayer;
    TaskCompletionSource? playbackFinished;
    public async Task StopPlayingAsync(){var done=playbackFinished?.Task;StopPlaying();if(done is not null)await done;}
    public event Action<TransportTick>? TransportTicked;
    public Guid? TransportEpoch=>loopPlayer?.TransportEpoch;
    public void QueueTransportBoundary(long absoluteTick)=>(loopPlayer??throw new IOException("Transport is stopped.")).ScheduleBoundary(absoluteTick);
    public void CancelTransportBoundary()=>loopPlayer?.CancelBoundary();
    public double? TransportPosition=>loopPlayer?.Clock?.Position;
    public void ReplaceLoopAtBoundary(AppPattern pattern,int tempo)=>(loopPlayer??throw new IOException("Transport is stopped.")).ReplaceAtBoundary(pattern,tempo);
    public event Action? TransportStopping;
    public void SendAutomationCc(int channel,int controller,int value)
    {
        if(channel is <0 or >15||controller is <0 or >127||value is <0 or >127)throw new ArgumentOutOfRangeException();
        lock(playbackSync){if(playback?.IsCancellationRequested==false&&transport is Sloop.Protocol.IMidiTransport midi)
            midi.SendAsync(new byte[]{(byte)(0xb0+channel),(byte)controller,(byte)value},playback.Token).GetAwaiter().GetResult();}
    }
    public Task LoopPatternAsync(AppPattern pattern,int tempo)=>RunLoopAsync(pattern,tempo);
    async Task RunLoopAsync(AppPattern pattern,int tempo)
    {
        PatternValidation.Validate(pattern);
        if(IsPlaying||IsRecording||Performer.Active||Snapshot.Busy||transport is not Sloop.Protocol.IMidiTransport midi||epoch is null||Snapshot.IsSimulated)throw new IOException("Connect MIDI and release performance before playback.");
        using var current=CancellationTokenSource.CreateLinkedTokenSource(epoch.Token);
        var player=new PatternLoop();var reservedEpoch=player.TransportEpoch;
        var completed=new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);playbackFinished=completed;
        loopPlayer=player;playback=current;Changed?.Invoke();
        player.Tick+=tick=>TransportTicked?.Invoke(tick);
        try { await Task.Run(()=>player.RunAsync(pattern,tempo,bytes=> {
            lock(playbackSync) {current.Token.ThrowIfCancellationRequested();var key=((int)(bytes[0]&15),(int)bytes[1]);
                if((bytes[0]&0xf0)==0x90)performanceNotes.Add(key);
                midi.SendAsync(bytes,current.Token).GetAwaiter().GetResult();
                if((bytes[0]&0xf0)==0x80)performanceNotes.Remove(key);
            }
        },current.Token,reservedEpoch)); }
        catch(OperationCanceledException)when(current.IsCancellationRequested){}
        finally {lock(playbackSync) {foreach(var key in performanceNotes.ToArray())try{midi.SendAsync(new byte[]{(byte)(0x80+key.Channel),(byte)key.Pitch,0},CancellationToken.None).GetAwaiter().GetResult();}catch(Exception){}performanceNotes.Clear();if(playback==current){playback=null;loopPlayer=null;}}try{Changed?.Invoke();}finally{completed.TrySetResult();}}
    }
}


