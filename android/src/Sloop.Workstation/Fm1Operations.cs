using Sloop.Protocol;
using Sloop.SampleEncoding;
using Sloop.SoundDesign;

namespace Sloop.Workstation;
public static class Fm1Operations
{
    public static async Task<Patch> ReadPatch(EditorClient client,byte track,CancellationToken token)
    {
        if(track>2) throw new ArgumentOutOfRangeException(nameof(track));
        var r=await client.RequestAsync(68,[0,track],f=>f.Arguments.Length>=3&&f.Arguments[0]==0&&f.Arguments[1]==track,cancellationToken:token);
        if(r.Arguments[2]!=0) throw new IOException($"FM6 read failed ({r.Arguments[2]}).");
        return PatchCodec.Unpack(r.Arguments.AsSpan(3));
    }
    public static async Task WritePatch(EditorClient client,byte track,Patch patch,CancellationToken token)
    {
        var r=await client.RequestAsync(69,[0,track,..PatchCodec.Pack(patch)],f=>f.Arguments.Length==3&&f.Arguments[0]==0&&f.Arguments[1]==track,cancellationToken:token);
        if(r.Arguments[2]!=0) throw new IOException($"FM6 write failed ({r.Arguments[2]}).");
        if(!PatchCodec.ContentEquals(patch,await ReadPatch(client,track,token))) throw new IOException("Patch readback differs. Hardware state is uncertain.");
    }
    public static byte[] Number(ulong number,int count=5) => Enumerable.Range(0,count).Select(i=>(byte)((number>>(7*i))&127)).ToArray();
    static ulong Number(ReadOnlySpan<byte> bytes) { ulong n=0; for(int i=0;i<bytes.Length;i++) n|=(ulong)bytes[i]<<(7*i); return n; }
    public static async Task<byte[]> BackupSlot(EditorClient client,byte slot,CancellationToken token)
    {
        byte id=(byte)(32+slot); var list=await client.RequestAsync(34,[],cancellationToken:token);
        var a=list.Arguments;
        if(a.Length<2 || a[0]!=0 || a.Length!=2+a[1]*11) throw new IOException("Invalid backup inventory.");
        int position=-1;
        for(int i=2;i<a.Length;i+=11) if(a[i]==id) position=i;
        if(position<0) throw new IOException("Slot absent from backup inventory.");
        int length=checked((int)Number(a.AsSpan(position+1,5))); uint crc=checked((uint)Number(a.AsSpan(position+6,5)));
        if(length==0) return [];
        if(length<512 || length>SlotBuilder.SlotBytes) throw new IOException("Invalid slot backup size.");
        var data=new byte[length];
        for(int off=0;off<length;off+=256) {
            int count=Math.Min(256,length-off); int expected=off;
            var r=await client.RequestAsync(35,[id,..Number((ulong)off),..Number((ulong)count,2)],f=>f.Arguments.Length>=9&&f.Arguments[0]==id&&Number(f.Arguments.AsSpan(2,5))==(ulong)expected,cancellationToken:token);
            var b=r.Arguments;
            if(b[1]!=0 || Number(b.AsSpan(7,2))!=(ulong)count) throw new IOException("Slot backup read failed.");
            var chunk=EditorCodec.Unpack7(b.AsSpan(9));
            if(chunk.Length!=count) throw new IOException("Truncated slot backup.");
            chunk.CopyTo(data,off);
        }
        if(SlotBuilder.Crc32(data)!=crc) throw new IOException("Slot backup CRC mismatch. Replacement cancelled.");
        return data;
    }
    public static async Task UploadSlot(EditorClient client,byte slot,SlotArtifact artifact,Action<int> progress,CancellationToken token)
    {
        if(slot>3 || !artifact.Fit.Fits || artifact.Header.Length!=480 || artifact.Data.Length>SlotBuilder.Capacity) throw new ArgumentException("Invalid slot artifact.");
        async Task Check(byte command,byte[] args,int length,int status) {
            var r=await client.RequestAsync(command,args,f=>f.Arguments.Length==length&&f.Arguments[0]==slot,
                timeout:TimeSpan.FromSeconds(10),cancellationToken:token);
            if(r.Arguments[status]!=0) throw new IOException($"Sample command {command} failed ({r.Arguments[status]}).");
            if(command==12 && !r.Arguments.AsSpan(1,3).SequenceEqual(args.AsSpan(1,3))) throw new IOException("Sample write offset mismatch.");
        }
        await Check(11,[slot],2,1);
        for(int off=0;off<artifact.Data.Length;off+=256) {
            int count=Math.Min(256,artifact.Data.Length-off);
            await Check(12,[slot,..Number((ulong)(512+off),3),..EditorCodec.Pack7(artifact.Data.AsSpan(off,count))],5,4);
            progress(off+count);
        }
        await Check(13,[slot,..EditorCodec.Pack7(artifact.Header)],2,1);
        var actual=await BackupSlot(client,slot,token);
        // Reserved header/data padding is not uploaded content.
        if(actual.Length!=artifact.Image.Length || !actual.AsSpan(0,480).SequenceEqual(artifact.Header) || !actual.AsSpan(512).SequenceEqual(artifact.Data))
            throw new IOException("Sample readback differs. Keep the saved backup and reconnect before retrying.");
    }
}
