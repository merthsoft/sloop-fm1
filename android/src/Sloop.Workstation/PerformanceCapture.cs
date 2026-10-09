using System.Collections.Immutable;
using Sloop.Sequencing;
namespace Sloop.Workstation;
public enum CaptureMode { Overdub, ReplaceChannels }

/// <summary>Captures emitted MIDI, including arp gates and retained shared pitches. Held notes split at loop wrap.</summary>
public sealed class PerformanceCapture(AppPattern source, CaptureMode mode, int tempo, int countInBars=0)
{
    readonly object sync=new();
    readonly MusicalClock clock=new(source.TicksPerQuarter,tempo);
    readonly long countIn=countInBars is >=0 and <=4 ? countInBars*source.TicksPerQuarter*4L : throw new ArgumentOutOfRangeException(nameof(countInBars));
    readonly Dictionary<LiveNote,(long Start,int Velocity)> active=[];
    readonly List<AppNote> captured=[];
    bool finished;
    public void SetTempo(int tempo)=>clock.SetTempo(tempo);
    public long Position=>Math.Max(0,(long)clock.Position-countIn);
    public bool CountingIn=>clock.Position<countIn;
    public void Observe(byte[] midi) => ObserveAt(midi,(long)clock.Position-countIn);
    public void ObserveAt(byte[] midi,long tick)
    {
        if(midi.Length<3)return;
        lock(sync) {
            if(finished)return; int kind=midi[0]&0xf0; var key=new LiveNote(midi[0]&15,midi[1]);
            if(kind==0x90&&midi[2]>0) { if(!active.ContainsKey(key))active[key]=(Math.Max(0,tick),midi[2]); }
            else if(kind==0x80||kind==0x90&&midi[2]==0) { if(active.Remove(key,out var value)&&tick>0)Close(key,value,tick); }
        }
    }
    void Close(LiveNote key,(long Start,int Velocity) value,long end)
    {
        long start=value.Start; end=Math.Max(start+1,end);
        // A held note longer than one phrase becomes one full phrase, not unbounded duplicate notes.
        end=Math.Min(end,checked(start+source.Length.Value));
        while(start<end) { long onset=start%source.Length.Value; long length=Math.Min(end-start,source.Length.Value-onset);captured.Add(new(Guid.NewGuid(),source.Id,new(onset),new(length),key.Pitch,value.Velocity,key.Channel));start+=length; }
    }
    public EditProposal Finish()=>FinishAt(Position);
    public EditProposal FinishAt(long tick)
    {
        lock(sync) {
            if(finished)throw new InvalidOperationException("Capture already finished.");finished=true;
            if(tick>0)foreach(var pair in active)Close(pair.Key,pair.Value,tick);active.Clear();
            var channels=captured.Select(n=>n.Channel).ToHashSet();
            var retained=mode==CaptureMode.ReplaceChannels?source.Notes.Where(n=>!channels.Contains(n.Channel)):source.Notes;
            var after=source with{Revision=Guid.NewGuid(),Notes=retained.Concat(captured).ToImmutableArray()};
            return EditProposal.Between(source,after,new("Record Perform gestures", "perform-capture-v1"));
        }
    }
}
