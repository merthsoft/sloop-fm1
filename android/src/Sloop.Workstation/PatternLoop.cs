using System.Diagnostics;
using Sloop.Sequencing;

namespace Sloop.Workstation;

/// <summary>Absolute ticks never wrap; LoopTick wraps at phrase length. Boundaries precede notes at that tick.</summary>
public readonly record struct TransportTick(long AbsoluteTick, long LoopTick, long Loop, int TicksPerQuarter, long PatternLength, Guid TransportEpoch, long PhraseOrigin = 0)
{
    public bool IsBeat => AbsoluteTick % TicksPerQuarter == 0;
    public bool IsBar => AbsoluteTick % (TicksPerQuarter * 4L) == 0;
    public bool IsLoop => LoopTick == 0;
}

/// <summary>Monotonic musical clock. Tempo changes preserve the current musical position.</summary>
public sealed class MusicalClock(int ppq, int tempo)
{
    readonly object sync = new();
    readonly Stopwatch watch = Stopwatch.StartNew();
    double anchorMilliseconds, anchorTick;
    int bpm = ppq>0 && tempo is >=30 and <=240 ? tempo : throw new ArgumentOutOfRangeException(nameof(tempo));
    public double Position { get { lock(sync) return PositionCore(); } }
    double PositionCore() => anchorTick + (watch.Elapsed.TotalMilliseconds-anchorMilliseconds)*bpm*ppq/60000.0;
    public void SetTempo(int value) { if(value is <30 or >240) throw new ArgumentOutOfRangeException(nameof(value)); lock(sync) { anchorTick=PositionCore(); anchorMilliseconds=watch.Elapsed.TotalMilliseconds; bpm=value; } }
    public async Task Until(long tick, CancellationToken token)
    {
        while(true) { token.ThrowIfCancellationRequested(); double remaining; lock(sync) remaining=(tick-PositionCore())*60000.0/(bpm*ppq); if(remaining<=0)return; await Task.Delay(Math.Max(1,(int)Math.Min(10,remaining)),token).ConfigureAwait(false); }
    }
}

public sealed class PatternLoop
{
    private sealed record Replacement(AppPattern Pattern,int Tempo);
    private Replacement? replacement;
    private bool inBoundary;
    private int currentPpq;
    public void ReplaceAtBoundary(AppPattern pattern,int tempo)
    {
        PatternValidation.Validate(pattern);
        if(!inBoundary||pattern.TicksPerQuarter!=currentPpq||tempo is <30 or >240)
            throw new InvalidOperationException("Replace a validated pattern at a transport boundary with the same PPQ.");
        replacement=new(pattern,tempo);
    }
    long scheduledBoundary=-1;
    public Guid TransportEpoch { get; private set; } = Guid.NewGuid();
    public event Action<TransportTick>? Tick;
    public void ScheduleBoundary(long absoluteTick)
    {
        if(Clock is null||absoluteTick<=Clock.Position)throw new InvalidOperationException("Queue a future boundary while transport is running.");
        Interlocked.Exchange(ref scheduledBoundary,absoluteTick);
    }
    public void CancelBoundary()=>Interlocked.Exchange(ref scheduledBoundary,-1);
    public MusicalClock? Clock { get; private set; }
    public async Task RunAsync(AppPattern pattern, int tempo, Action<byte[]> send, CancellationToken token, Guid? reservedEpoch = null)
    {
        PatternValidation.Validate(pattern);
        var active = new Dictionary<LiveNote,int>();
        if(reservedEpoch == Guid.Empty)throw new ArgumentException("Transport epoch must be nonempty.",nameof(reservedEpoch));
        TransportEpoch=reservedEpoch ?? Guid.NewGuid();
        currentPpq=pattern.TicksPerQuarter;
        Clock=new(pattern.TicksPerQuarter,tempo);

        try {
            long origin=0;
            for(long loop=0;;loop++) {
                var events=pattern.Notes.SelectMany(n=>new[]{(At:n.Start.Value,On:true,Note:n),(At:n.Start.Value+n.Duration.Value,On:false,Note:n)}).OrderBy(e=>e.At).ThenBy(e=>e.On).ToArray();
                int index=0; long boundary=0, boundaryIndex=0; bool replaced=false;
                while(true) {
                    long queued, due, next;
                    // Re-evaluate the pending scene while waiting: queue/cancel may arrive from another thread.
                    while(true) {
                        token.ThrowIfCancellationRequested();
                        queued=Interlocked.Read(ref scheduledBoundary);
                        due=queued>=origin&&queued<origin+pattern.Length.Value?queued-origin:long.MaxValue;
                        next=Math.Min(pattern.Length.Value,Math.Min(due,Math.Min(index<events.Length?events[index].At:long.MaxValue,boundary<pattern.Length.Value?boundary:long.MaxValue)));
                        if(next==long.MaxValue||Clock.Position>=checked(origin+next))break;
                        await Task.Delay(1,token).ConfigureAwait(false);
                    }
                    if(next==long.MaxValue)break;
                    bool regular=next==boundary&&boundary<pattern.Length.Value;
                    if(next==due)Interlocked.CompareExchange(ref scheduledBoundary,-1,queued);
                    if(regular||next==due) {
                        inBoundary=true;
                        try{Tick?.Invoke(new(origin+next,next,loop,pattern.TicksPerQuarter,pattern.Length.Value,TransportEpoch,origin));}
                        finally{inBoundary=false;}
                    }
                    token.ThrowIfCancellationRequested();
                    if(replacement is {} change) {
                        replacement=null;
                        foreach(var key in active.Keys)send([(byte)(0x80+key.Channel),(byte)key.Pitch,0]);
                        active.Clear();
                        origin=checked(origin+next);pattern=change.Pattern;Clock.SetTempo(change.Tempo);replaced=true;
                        break; // Drop unsent outgoing events; new phrase begins at this exact absolute tick.
                    }
                    if(regular)boundary=pattern.TicksPerQuarter<4?boundary+1:checked((++boundaryIndex*pattern.TicksPerQuarter+3)/4);
                    while(index<events.Length&&events[index].At==next) {
                        token.ThrowIfCancellationRequested();var e=events[index++];var key=new LiveNote(e.Note.Channel,e.Note.Pitch);active.TryGetValue(key,out int count);
                        if(e.On) {active[key]=count+1;if(count==0)send([(byte)(0x90+key.Channel),(byte)key.Pitch,(byte)e.Note.Velocity]);}
                        else if(count>0) {if(count==1){send([(byte)(0x80+key.Channel),(byte)key.Pitch,0]);active.Remove(key);}else active[key]=count-1;}
                    }
                    if(next==pattern.Length.Value)break;
                }
                if(!replaced){origin=checked(origin+pattern.Length.Value);await Clock.Until(origin,token).ConfigureAwait(false);}
            }
        } finally { foreach(var key in active.Keys)try{send([(byte)(0x80+key.Channel),(byte)key.Pitch,0]);}catch(Exception){} replacement=null;CancelBoundary();Clock=null; }
    }
}






