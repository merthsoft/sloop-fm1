namespace Sloop.Workstation;

public readonly record struct MidiChannelMessage(byte Status, byte Data1, byte Data2);

/// <summary>MIDI 1 byte-stream decoder. Realtime bytes do not interrupt running status;
/// system common and SysEx clear it. Only complete channel messages are emitted.</summary>
public sealed class MidiInputDecoder(Action<MidiChannelMessage> receive)
{
    byte status, first; int count; bool sysex;
    public void Reset() { status=first=0;count=0;sysex=false; }
    public void Feed(ReadOnlySpan<byte> bytes)
    {
        foreach(var b in bytes) {
            if(b>=0xf8)continue;
            if(b>=0x80) {
                count=0;
                if(b>=0xf0){status=0;sysex=b==0xf0;continue;}
                sysex=false;status=b;continue;
            }
            if(sysex||status==0)continue;
            if((status&0xf0) is 0xc0 or 0xd0){receive(new(status,b,0));continue;}
            if(count++==0){first=b;continue;}
            count=0;receive(new(status,first,b));
        }
    }
}

/// <summary>Single-threaded controller ownership. Repeated presses share one owner,
/// with balanced release counts; sustain is per source channel.</summary>
public sealed class ExternalMidiNotes(Action<string,int,int> hold, Action<string> release)
{
    readonly Dictionary<(int Channel,int Pitch),int> pressed=[];
    readonly HashSet<(int Channel,int Pitch)> sounding=[];
    readonly bool[] sustain=new bool[16];
    static string Owner((int Channel,int Pitch) n)=>$"external:{n.Channel}:{n.Pitch}";
    public int Count=>sounding.Count;
    public void Receive(MidiChannelMessage m)
    {
        int ch=m.Status&15,kind=m.Status&0xf0;var key=(ch,(int)m.Data1);
        if(kind==0x90&&m.Data2>0) {
            pressed.TryGetValue(key,out int n);pressed[key]=n+1;
            if(sounding.Add(key))hold(Owner(key),m.Data1,m.Data2);
        } else if(kind==0x80||kind==0x90) {
            if(!pressed.TryGetValue(key,out int n))return;
            if(n>1){pressed[key]=n-1;return;}pressed.Remove(key);
            if(!sustain[ch])Release(key);
        } else if(kind==0xb0) {
            if(m.Data1==64){sustain[ch]=m.Data2>=64;if(!sustain[ch])foreach(var k in sounding.Where(k=>k.Channel==ch&&!pressed.ContainsKey(k)).ToArray())Release(k);}
            if(m.Data1 is 120 or 123){foreach(var k in sounding.Where(k=>k.Channel==ch).ToArray())Release(k);foreach(var k in pressed.Keys.Where(k=>k.Channel==ch).ToArray())pressed.Remove(k);sustain[ch]=false;}
            if(m.Data1==121){sustain[ch]=false;foreach(var k in sounding.Where(k=>k.Channel==ch&&!pressed.ContainsKey(k)).ToArray())Release(k);}
        }
    }
    void Release((int Channel,int Pitch) key){if(sounding.Remove(key))release(Owner(key));}
    public void Reset(){var active=sounding.ToArray();sounding.Clear();pressed.Clear();Array.Clear(sustain);foreach(var k in active)try{release(Owner(k));}catch(Exception){/* Best effort on a removed transport; ownership is still cleared. */}}
}

public static class ExternalMidiHarmony
{
    /// <summary>Exact in-scale roots only. Register comes from the controller pitch.</summary>
    public static ChordVoicing? Chord(int pitch,int key,PerformanceScale scale,ChordShape shape,int inversion)
    {
        var harmony=scale==PerformanceScale.MajorPentatonic?PerformanceScale.Major:scale==PerformanceScale.MinorPentatonic?PerformanceScale.NaturalMinor:scale;
        int relative=pitch-key,degree=Array.IndexOf(PerformanceHarmony.Scale(harmony),((relative%12)+12)%12);
        if(degree<0)return null;
        int octave=(int)Math.Floor(relative/12.0)-1;
        if(octave is < -1 or >7)return null;
        return PerformanceHarmony.Chord(key,harmony,octave,degree,shape,inversion,false);
    }
}
