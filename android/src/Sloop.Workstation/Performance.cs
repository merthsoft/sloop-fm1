namespace Sloop.Workstation;

public enum PerformanceScale { Major, NaturalMinor, Dorian, Mixolydian, HarmonicMinor, MajorPentatonic, MinorPentatonic }
public enum ChordShape { Diatonic, Seventh, Ninth, Sus4, Sus2, Minor, Major, InversionDown, InversionUp }
public enum PlayStyle { Block, StrumUp, StrumDown, ArpUp, ArpDown, ArpBounce, ArpRandom, Repeat }
public readonly record struct LiveNote(int Channel,int Pitch);
public sealed record ChordVoicing(string Name,int[] Notes,int Bass);

public static class PerformanceHarmony
{
    public static int EffectiveOctave(int appOctave, int? hardwareOffset, bool followHardware)
    {
        if(appOctave is <1 or >6 || hardwareOffset is < -3 or >3) throw new ArgumentOutOfRangeException(nameof(appOctave));
        return Math.Clamp(appOctave+(followHardware?hardwareOffset??0:0),-1,7);
    }
    public static readonly string[] Names=["C","C♯","D","E♭","E","F","F♯","G","A♭","A","B♭","B"];
    public static int[] Scale(PerformanceScale scale)=>scale switch {
        PerformanceScale.Major=>[0,2,4,5,7,9,11], PerformanceScale.NaturalMinor=>[0,2,3,5,7,8,10],
        PerformanceScale.Dorian=>[0,2,3,5,7,9,10],PerformanceScale.Mixolydian=>[0,2,4,5,7,9,10],
        PerformanceScale.HarmonicMinor=>[0,2,3,5,7,8,11],PerformanceScale.MajorPentatonic=>[0,2,4,7,9],
        PerformanceScale.MinorPentatonic=>[0,3,5,7,10],_=>throw new ArgumentOutOfRangeException(nameof(scale))};
    public static int ScaleNote(int key,PerformanceScale scale,int octave,int degree) {
        if(key is <0 or >11||octave is < -1 or >7||degree<0)throw new ArgumentOutOfRangeException(nameof(degree));
        var s=Scale(scale);return Math.Clamp(12*(octave+1)+key+s[degree%s.Length]+12*(degree/s.Length),0,127);
    }
    public static ChordVoicing Chord(int key,PerformanceScale scale,int octave,int degree,ChordShape shape,int inversion,bool voiceLead,int[]? previous=null) {
        if(degree is <0 or >7||inversion is <-3 or >3)throw new ArgumentOutOfRangeException(nameof(degree));
        // Eight chord buttons include the upper tonic; pentatonic scales affect the melody grid.
        var harmony=scale==PerformanceScale.MajorPentatonic?PerformanceScale.Major:scale==PerformanceScale.MinorPentatonic?PerformanceScale.NaturalMinor:scale;
        int root=ScaleNote(key,harmony,octave,degree);
        int third=ScaleNote(key,harmony,octave,degree+2)-root,fifth=ScaleNote(key,harmony,octave,degree+4)-root;
        int seventh=ScaleNote(key,harmony,octave,degree+6)-root,ninth=ScaleNote(key,harmony,octave,degree+8)-root;
        int[] intervals=shape switch {ChordShape.Seventh=>[0,third,fifth,seventh],ChordShape.Ninth=>[0,third,seventh,ninth],
            ChordShape.Sus4=>[0,5,7],ChordShape.Sus2=>[0,2,7],ChordShape.Minor=>[0,3,7],ChordShape.Major=>[0,4,7],_=>[0,third,fifth]};
        string quality=shape switch {ChordShape.Seventh=>third==3?(fifth==6?(seventh==9?"dim7":"m7♭5"):seventh==11?"m(maj7)":"m7"):seventh==11?"maj7":"7",ChordShape.Ninth=>third==3?(seventh==11?"m(maj9)":"m9"):seventh==11?"maj9":"9",
            ChordShape.Sus4=>"sus4",ChordShape.Sus2=>"sus2",ChordShape.Minor=>"m",ChordShape.Major=>"",_=>third==3?(fifth==6?"dim":"m"):fifth==8?"aug":""};
        var notes=intervals.Select(n=>root+n).ToArray();
        for(int i=0;i<Math.Abs(inversion);i++){if(inversion>0){notes[0]+=12;Array.Sort(notes);}else{notes[^1]-=12;Array.Sort(notes);}}
        if(voiceLead&&previous is {Length:>0}) {
            var candidates=new List<int[]>();
            // Keep automatic voice leading in the requested register. Explicit inversions retain
            // their own bass octave rather than being undone by an automatic octave shift.
            int floor=inversion==0?12*(octave+1)+key+12*(degree/7):(int)Math.Floor(notes[0]/12.0)*12;
            for(int rotate=0;rotate<(inversion==0?notes.Length:1);rotate++)for(int shift=-12;shift<=12;shift+=12){var c=notes.Select((n,i)=>n+shift+(i<rotate?12:0)).Order().ToArray();if(c.All(n=>n is >=0 and <=127)&&c[0]>=floor&&c[0]<floor+12&&(inversion!=0||c.Contains(root)))candidates.Add(c);}
            if(candidates.Count>0)notes=candidates.OrderBy(c=>c.Select((n,i)=>Math.Abs(n-previous[Math.Min(i,previous.Length-1)])).Sum()).ThenBy(c=>c[0]).First();
        }
        return new(Names[root%12]+quality,notes.Select(n=>Math.Clamp(n,0,127)).Distinct().Order().ToArray(),Math.Clamp(root-12,0,127));
    }
    public static int ArpIndex(PlayStyle style,int beat,int count,int seed=42) {
        if(count<1||beat<0)throw new ArgumentOutOfRangeException(nameof(count));
        return style switch {PlayStyle.ArpDown=>count-1-beat%count,PlayStyle.ArpBounce=>count==1?0:count-1-Math.Abs(beat%(count*2-2)-(count-1)),
            PlayStyle.ArpRandom=>(int)((uint)(beat*1664525+seed*1013904223)%count),_=>beat%count};
    }
}

/// <summary>Serializes MIDI ownership; overlapping fingers retain shared pitches.</summary>
public sealed class LiveNotes(Action<byte[]> send)
{
    readonly Dictionary<string,HashSet<LiveNote>> owners=[];
    readonly Dictionary<LiveNote,int> counts=[];
    public int Count=>counts.Count;
    public void Set(string owner,IEnumerable<LiveNote> notes,int velocity) {
        if(velocity is <1 or >127)throw new ArgumentOutOfRangeException(nameof(velocity));
        var next=notes.ToHashSet();if(next.Any(n=>n.Channel is <0 or >15||n.Pitch is <0 or >127))throw new ArgumentOutOfRangeException(nameof(notes));
        owners.TryGetValue(owner,out var old);old??=[];
        foreach(var n in old.Except(next).ToArray()){if(--counts[n]==0){counts.Remove(n);send([(byte)(0x80+n.Channel),(byte)n.Pitch,0]);}}
        foreach(var n in next.Except(old).ToArray()){counts.TryGetValue(n,out var count);counts[n]=count+1;if(count==0)send([(byte)(0x90+n.Channel),(byte)n.Pitch,(byte)velocity]);}
        if(next.Count==0)owners.Remove(owner);else owners[owner]=next;
    }
    public void Panic() {
        var active=counts.Keys.ToArray();owners.Clear();counts.Clear();
        foreach(var n in active)try{send([(byte)(0x80+n.Channel),(byte)n.Pitch,0]);}catch(Exception){}
    }
}

/// <summary>Generations cancel pending strums and repeats. Timed modes run off the UI thread.</summary>
public sealed class PerformancePlayer(Action<byte[]> send):IDisposable
{
    readonly object sync=new();readonly LiveNotes notes=new(send);
    readonly Dictionary<string,CancellationTokenSource> jobs=[];
    public event Action<Exception>? Failed;
    public bool Active {get{lock(sync)return jobs.Count>0||notes.Count>0;}}
    public void Hold(string owner,LiveNote[] chord,int velocity,PlayStyle style,int bpm=100,int division=4,int strumMs=35,LiveNote? bass=null,bool retrigger=false) {
        if(bpm is <30 or >240||division is <1 or >8||strumMs is <5 or >200||velocity is <1 or >127||chord.Length==0||chord.Any(n=>n.Channel is <0 or >15||n.Pitch is <0 or >127)||bass is {} b&&(b.Channel is <0 or >15||b.Pitch is <0 or >127))throw new ArgumentOutOfRangeException(nameof(chord));
        lock(sync) {
            if(style==PlayStyle.Block) {
                Cancel(owner);
                // A deliberate chord attack releases this owner's old voicing first.
                // Notes held by another owner remain sustained.
                if(retrigger)notes.Set(owner,[],velocity);
                notes.Set(owner,chord.Concat(bass is {} n?new[]{n}:[]),velocity);return;
            }
            Release(owner);var cts=new CancellationTokenSource();jobs[owner]=cts;
            _=Task.Run(async()=> {
                try {
                    var token=cts.Token;var clock=System.Diagnostics.Stopwatch.StartNew();
                    async Task Until(double ms){while(clock.Elapsed.TotalMilliseconds<ms){token.ThrowIfCancellationRequested();await Task.Delay(Math.Max(1,(int)Math.Min(10,ms-clock.Elapsed.TotalMilliseconds)),token);}}
                    void Emit(LiveNote[] value){lock(sync){token.ThrowIfCancellationRequested();if(!jobs.TryGetValue(owner,out var current)||current!=cts)throw new OperationCanceledException();notes.Set(owner,value.Concat(bass is {} n?new[]{n}:[]),velocity);}}
                    if(style is PlayStyle.StrumUp or PlayStyle.StrumDown){var order=style==PlayStyle.StrumDown?chord.Reverse().ToArray():chord;for(int i=0;i<order.Length;i++){await Until(i*strumMs);Emit(order.Take(i+1).ToArray());}await Task.Delay(Timeout.Infinite,token);}
                    else {double step=60000.0/bpm/division;for(int beat=0;;beat++){await Until(beat*step);Emit(style==PlayStyle.Repeat?chord:[chord[PerformanceHarmony.ArpIndex(style,beat,chord.Length)]]);await Until(beat*step+step*.7);Emit([]);}}
                }catch(OperationCanceledException){}catch(Exception e){Failed?.Invoke(e);}
                finally{lock(sync){if(jobs.TryGetValue(owner,out var current)&&current==cts){jobs.Remove(owner);try{notes.Set(owner,[],velocity);}catch(Exception){} }cts.Dispose();}}
            });
        }
    }
    void Cancel(string owner){if(jobs.Remove(owner,out var cts))cts.Cancel();}
    public void Release(string owner){lock(sync){Cancel(owner);notes.Set(owner,[],100);}}
    public void Panic(){lock(sync){foreach(var cts in jobs.Values)cts.Cancel();jobs.Clear();notes.Panic();}}
    public void Dispose()=>Panic();
}
