using System.Collections.Immutable;

namespace Sloop.SoundDesign;

/// <summary>Preserves the exact imported asset separately from the editable canonical voices.</summary>
public sealed record ImportedSysEx(ImmutableArray<byte> Original, ImmutableArray<Patch> Voices, int Channel)
{
    public byte[] ExportOriginal() => Original.ToArray();
}
public static class SysExCodec
{
    public static byte Checksum(ReadOnlySpan<byte> payload)
    {
        int sum=0;foreach(byte b in payload)sum+=b;
        return (byte)((-sum)&127);
    }
    /// <summary>Accepts exactly one complete single-voice or 32-voice bank message.</summary>
    public static ImportedSysEx Import(ReadOnlySpan<byte> message)
    {
        if(message.Length<8 || message[0]!=0xf0 || message[1]!=0x43 || message[2]>15 || message[^1]!=0xf7)
            throw new ArgumentException("Unsupported or incomplete voice SysEx header.");
        int length=message[3] switch {0=>155,9=>4096,_=>throw new ArgumentException("Unsupported SysEx voice format.")};
        if(message.Length!=length+8 || message[4]!=(length>>7) || message[5]!=(length&127))
            throw new ArgumentException("SysEx size does not match its format/count.");
        var payload=message.Slice(6,length);
        foreach(byte b in payload)if(b>127)throw new ArgumentException("SysEx payload is not 7-bit.");
        if(message[^2]!=Checksum(payload))throw new ArgumentException("Invalid voice SysEx checksum.");
        var voices=ImmutableArray.CreateBuilder<Patch>();
        if(length==155)voices.Add(PatchCodec.DecodeVoice(payload));
        else for(int i=0;i<32;i++)voices.Add(PatchCodec.Unpack(payload.Slice(i*128,128)));
        return new(message.ToArray().ToImmutableArray(),voices.ToImmutable(),message[2]);
    }
    public static byte[] ExportSingle(Patch patch,int channel=0)=>Wrap(PatchCodec.EncodeVoice(patch),0,channel);
    /// <summary>No implicit padding or truncation: a bank must have exactly 32 validated voices.</summary>
    public static byte[] ExportBank(IEnumerable<Patch> patches,int channel=0)
    {
        var voices=patches.ToImmutableArray();
        if(voices.Length!=32)throw new ArgumentException("A bank must contain exactly 32 voices.");
        var payload=new byte[4096];for(int i=0;i<32;i++)PatchCodec.Pack(voices[i]).CopyTo(payload,i*128);
        return Wrap(payload,9,channel);
    }
    static byte[] Wrap(byte[] payload,byte format,int channel)
    {
        if(channel is < 0 or > 15)throw new ArgumentOutOfRangeException(nameof(channel));
        var message=new byte[payload.Length+8];
        message[0]=0xf0;message[1]=0x43;message[2]=(byte)channel;message[3]=format;
        message[4]=(byte)(payload.Length>>7);message[5]=(byte)(payload.Length&127);
        payload.CopyTo(message,6);message[^2]=Checksum(payload);message[^1]=0xf7;return message;
    }
}
