using System.Collections.Immutable;
using Sloop.Sequencing;

namespace Sloop.Workstation;
/// <summary>Native v5 step layouts. Preserve every inactive slot/lane and two-bit level/ratchet.</summary>
public static class HardwareSteps
{
    static uint Read(ReadOnlySpan<byte> bytes) {uint n=0;for(int i=0;i<bytes.Length;i++)n|=(uint)bytes[i]<<(7*i);return n;}
    public static byte[] Encode(HardwareStep step) => step switch {
        SynthStep s=>EncodeSynth(s),DrumStep d=>EncodeDrum(d),_=>throw new ArgumentException("Unknown step type.")
    };
    static byte[] EncodeSynth(SynthStep s)
    {
        uint level=0,rat=0;for(int i=0;i<4;i++){level|=(uint)s.Slots[i].Level<<(i*2);rat|=(uint)(s.Slots[i].Ratchet-1)<<(i*2);}
        return [(byte)s.Count,..s.Slots.Select(n=>(byte)n.Pitch),(byte)s.Time,(byte)s.Flags,(byte)s.Velocity,(byte)(level&127),(byte)((level>>7)|((rat>>7)<<1)),(byte)(rat&127)];
    }
    static byte[] EncodeDrum(DrumStep d)
    {
        uint on=0,level=0,rat=0;for(int i=0;i<16;i++){if(d.Lanes[i].On)on|=1u<<i;level|=(uint)d.Lanes[i].Level<<(i*2);rat|=(uint)(d.Lanes[i].Ratchet-1)<<(i*2);}
        return [..Fm1Operations.Number(on,3),..Fm1Operations.Number(level),..Fm1Operations.Number(rat)];
    }
    public static SynthStep DecodeSynth(ReadOnlySpan<byte> bytes,SynthStep identity,MicroOffset micro,FillCondition fill)
    {
        if(bytes.Length!=11||bytes[0]>4||bytes[5]>2||(bytes[6]&~3)!=0||bytes[9]>3)throw new FormatException("Invalid native synth step.");
        uint level=(uint)(bytes[8]|(bytes[9]&1)<<7),rat=(uint)(bytes[10]|(bytes[9]>>1)<<7);
        var raw=bytes.ToArray();
        return identity with {Count=bytes[0],Time=(StepTime)bytes[5],Flags=(StepFlags)bytes[6],Velocity=bytes[7],Micro=micro,Fill=fill,
            Slots=Enumerable.Range(0,4).Select(i=>identity.Slots[i] with{Pitch=raw[i+1],Level=(HitLevel)((level>>(2*i))&3),Ratchet=1+(int)((rat>>(2*i))&3)}).ToImmutableArray()};
    }
    public static DrumStep DecodeDrum(ReadOnlySpan<byte> bytes,DrumStep identity,MicroOffset micro,FillCondition fill)
    {
        if(bytes.Length!=13||bytes[2]>3||bytes[7]>15||bytes[12]>15)throw new FormatException("Invalid native drum step.");
        uint on=Read(bytes[..3]),level=Read(bytes.Slice(3,5)),rat=Read(bytes.Slice(8,5));
        return identity with{Micro=micro,Fill=fill,Lanes=Enumerable.Range(0,16).Select(i=>identity.Lanes[i] with{On=(on&(1u<<i))!=0,Level=(HitLevel)((level>>(2*i))&3),Ratchet=1+(int)((rat>>(2*i))&3)}).ToImmutableArray()};
    }
    public static bool ContentEquals(HardwareTrack a,HardwareTrack b) => a.Length==b.Length&&a.Capabilities.Identity==b.Capabilities.Identity&&
        a.Parameters.OrderBy(p=>p.Key).SequenceEqual(b.Parameters.OrderBy(p=>p.Key))&&
        a.Locks.OrderBy(l=>l.Step.Value).ThenBy(l=>l.Parameter).Select(l=>(l.Step,l.Parameter,l.Value)).SequenceEqual(b.Locks.OrderBy(l=>l.Step.Value).ThenBy(l=>l.Parameter).Select(l=>(l.Step,l.Parameter,l.Value)))&&
        a.Steps.Zip(b.Steps).All(pair=>pair.First.Micro==pair.Second.Micro&&pair.First.Fill==pair.Second.Fill&&Encode(pair.First).SequenceEqual(Encode(pair.Second)));
}
