using System.Buffers.Binary;
using Sloop.SampleEncoding;

namespace Sloop.Workstation;
public static class SlotImages
{
    public static SlotArtifact Read(byte[] bytes)
    {
        if(bytes.Length<512||bytes.Length>SlotBuilder.SlotBytes||BinaryPrimitives.ReadUInt32LittleEndian(bytes)!=0x504d5346||BinaryPrimitives.ReadUInt16LittleEndian(bytes.AsSpan(4))!=1||bytes[6] is <1 or >16)
            throw new FormatException("Invalid FM1 slot image.");
        int dataLength=checked((int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(16)));
        if(dataLength!=bytes.Length-512||SlotBuilder.Crc32(bytes.AsSpan(512))!=BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(20)))throw new FormatException("Slot size or data CRC mismatch.");
        long samples=0;var previews=new List<ZonePreview>();
        for(int zone=0;zone<bytes[6];zone++) {
            int p=32+zone*28; int offset=checked((int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(p)));
            int count=checked((int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(p+4)));
            int loopStart=checked((int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(p+8))),loopEnd=checked((int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(p+12)));
            int rate=checked((int)BinaryPrimitives.ReadUInt32LittleEndian(bytes.AsSpan(p+16)));
            int root=BinaryPrimitives.ReadInt16LittleEndian(bytes.AsSpan(p+20)); int lo=bytes[p+25],hi=bytes[p+26];
            if(count<1||offset<0||offset>dataLength||((long)count+1)/2>dataLength-offset||lo>hi||loopStart<0||loopEnd>=count||loopStart>loopEnd||bytes[p+24]>88||bytes[p+27]>1||rate is <1 or >262144)
                throw new FormatException("Invalid slot zone metadata.");
            samples+=count;previews.Add(new(zone,root/16,lo,hi,offset,ImaAdpcm.Decode(bytes.AsSpan(512+offset),count)));
        }
        return new(bytes[..480],bytes[512..],bytes.ToArray(),new(samples,dataLength,SlotBuilder.Capacity),previews.AsReadOnly());
    }
}
