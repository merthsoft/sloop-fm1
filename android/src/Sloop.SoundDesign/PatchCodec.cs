using System.Collections.Immutable;

namespace Sloop.SoundDesign;

/// <summary>Raw voices only, without SysEx headers or pack7. Invalid parameters are rejected, never clamped.</summary>
public static class PatchCodec
{
    static int[] Eg(Envelope e) => [e.Rate1,e.Rate2,e.Rate3,e.Rate4,e.Level1,e.Level2,e.Level3,e.Level4];
    internal static int[] Values(Patch p)
    {
        var v = new List<int>(155);
        foreach (var o in p.Operators.Reverse())
        {
            v.AddRange(Eg(o.Envelope));
            v.AddRange([o.Breakpoint,o.LeftDepth,o.RightDepth,(int)o.LeftCurve,(int)o.RightCurve,o.RateScaling,
                o.AmplitudeSensitivity,o.VelocitySensitivity,o.OutputLevel,(int)o.Mode,o.Coarse,o.Fine,o.Detune]);
        }
        v.AddRange(Eg(p.PitchEnvelope));
        v.AddRange([p.Algorithm-1,p.Feedback,p.OscillatorSync?1:0,p.LfoSpeed,p.LfoDelay,p.LfoPitchDepth,
            p.LfoAmplitudeDepth,p.LfoSync?1:0,(int)p.LfoWave,p.PitchSensitivity,p.Transpose]);
        v.AddRange(p.Name.PadRight(10).Select(c => (int)c));
        return v.ToArray();
    }
    public static byte[] EncodeVoice(Patch patch)
    {
        PatchValidation.Require(patch);
        return Values(patch).Select(v => (byte)v).ToArray();
    }
    public static Patch DecodeVoice(ReadOnlySpan<byte> voice)
    {
        if (voice.Length != 155) throw new ArgumentException("Voice must be 155 bytes.");
        var v = voice.ToArray();
        Envelope eg(int i) => new(v[i],v[i+1],v[i+2],v[i+3],v[i+4],v[i+5],v[i+6],v[i+7]);
        var ops = ImmutableArray.CreateBuilder<Operator>(6);
        for (int n = 5; n >= 0; n--)
        {
            int i = n * 21;
            ops.Add(new(eg(i),v[i+8],v[i+9],v[i+10],(ScalingCurve)v[i+11],(ScalingCurve)v[i+12],
                v[i+13],v[i+14],v[i+15],v[i+16],(FrequencyMode)v[i+17],v[i+18],v[i+19],v[i+20]));
        }
        if (v[136] > 1 || v[141] > 1) throw new ArgumentException("Sync flags must be 0 or 1.");
        var patch = new Patch(ops.ToImmutable(),eg(126),v[134]+1,v[135],v[136]!=0,v[137],v[138],v[139],
            v[140],v[141]!=0,(LfoWave)v[142],v[143],v[144],new string(v[145..].Select(b => (char)b).ToArray()).TrimEnd(' '));
        PatchValidation.Require(patch);
        return patch;
    }
    public static byte[] Pack(Patch patch)
    {
        var v = EncodeVoice(patch); var b = new byte[128];
        for (int k=0;k<6;k++)
        {
            int o=k*21,d=k*17;
            Array.Copy(v,o,b,d,11);
            b[d+11]=(byte)(v[o+11]|v[o+12]<<2);
            b[d+12]=(byte)(v[o+13]|v[o+20]<<3);
            b[d+13]=(byte)(v[o+14]|v[o+15]<<2);
            b[d+14]=v[o+16]; b[d+15]=(byte)(v[o+17]|v[o+18]<<1); b[d+16]=v[o+19];
        }
        Array.Copy(v,126,b,102,9); b[111]=(byte)(v[135]|v[136]<<3);
        Array.Copy(v,137,b,112,4); b[116]=(byte)(v[141]|v[142]<<1|v[143]<<4);
        b[117]=v[144]; Array.Copy(v,145,b,118,10); return b;
    }
    public static Patch Unpack(ReadOnlySpan<byte> packed)
    {
        if (packed.Length != 128) throw new ArgumentException("Packed voice must be 128 bytes.");
        var b=packed.ToArray(); var v=new byte[155];
        if (b.Any(x=>x>127)) throw new ArgumentException("Packed bytes must be 7-bit.");
        for(int k=0;k<6;k++)
        {
            int o=k*17,d=k*21; Array.Copy(b,o,v,d,11);
            v[d+11]=(byte)(b[o+11]&3); v[d+12]=(byte)((b[o+11]>>2)&3);
            v[d+13]=(byte)(b[o+12]&7); v[d+20]=(byte)((b[o+12]>>3)&15);
            v[d+14]=(byte)(b[o+13]&3); v[d+15]=(byte)((b[o+13]>>2)&7);
            v[d+16]=b[o+14]; v[d+17]=(byte)(b[o+15]&1); v[d+18]=(byte)((b[o+15]>>1)&31); v[d+19]=b[o+16];
        }
        Array.Copy(b,102,v,126,9); v[134]&=31; v[135]=(byte)(b[111]&7);v[136]=(byte)((b[111]>>3)&1);
        Array.Copy(b,112,v,137,4);v[141]=(byte)(b[116]&1);v[142]=(byte)((b[116]>>1)&7);v[143]=(byte)((b[116]>>4)&7);
        v[144]=b[117];Array.Copy(b,118,v,145,10);return DecodeVoice(v);
    }
    public static bool ContentEquals(Patch a, Patch b) => EncodeVoice(a).AsSpan().SequenceEqual(EncodeVoice(b));
}
