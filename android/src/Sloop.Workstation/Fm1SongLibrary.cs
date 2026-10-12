using System.Buffers.Binary;
using System.Text.Json;
using Sloop.Protocol;
using Sloop.SampleEncoding;
using Sloop.SoundDesign;

namespace Sloop.Workstation;

public sealed record SongObject(byte Id, byte[] Data, uint Crc);
public sealed record SongPatch(byte Track, byte[] Data,uint Crc);
public sealed record Fm1Song(Guid Id,string Name,string Firmware,DateTimeOffset CreatedUtc,
    int Parameters,string[] Engines,SongObject[] Objects,SongPatch[] Patches,int Version=1);

/// <summary>Native projects, sections and sound dependencies. Device settings are retained;
/// only the arrangement bytes are copied from the saved settings object.</summary>
public static class Fm1Songs
{
    const int MaxArchive=600_000;
    static readonly byte[] Required=[0,1,2,3,4,5,6,7,8,9,32,33,34,35];
    public static byte[] Encode(Fm1Song song) { Validate(song);return JsonSerializer.SerializeToUtf8Bytes(song); }
    public static Fm1Song Decode(Stream input)
    {
        using var buffer=new MemoryStream();var chunk=new byte[8192];int n;
        while((n=input.Read(chunk))!=0){if(buffer.Length+n>MaxArchive)throw new IOException("Song archive is too large.");buffer.Write(chunk,0,n);}
        var song=JsonSerializer.Deserialize<Fm1Song>(buffer.ToArray())??throw new IOException("Empty song archive.");Validate(song);return song;
    }
    public static void Validate(Fm1Song song)
    {
        if(song.Version!=1||song.Id==Guid.Empty||string.IsNullOrWhiteSpace(song.Name)||song.Name.Length>120||
            song.Parameters!=71||song.Engines is null||song.Engines.Length is <1 or >32||song.Engines.Any(string.IsNullOrWhiteSpace)||
            song.Objects is null||song.Objects.Length is <14 or >15||song.Patches is null||song.Patches.Length>3)
            throw new IOException("Unsupported or incomplete FM1 song archive.");
        if(song.Objects.Select(o=>o.Id).Distinct().Count()!=song.Objects.Length||Required.Any(id=>!song.Objects.Any(o=>o.Id==id)))
            throw new IOException("Song is missing objects or contains duplicates.");
        foreach(var o in song.Objects) {
            if(!Required.Contains(o.Id)&&o.Id!=10||o.Data is null||o.Data.Length>(o.Id>=32?SlotBuilder.SlotBytes:4096)||SlotBuilder.Crc32(o.Data)!=o.Crc)
                throw new IOException("Song object size or checksum is invalid.");
            if(o.Id==0||o.Id is >=2 and <=5&&o.Data.Length!=0) {
                if(o.Data.Length!=3816||BinaryPrimitives.ReadUInt32LittleEndian(o.Data)!=0x46554e36||
                    BinaryPrimitives.ReadUInt32LittleEndian(o.Data.AsSpan(4))!=3816)throw new IOException("Song requires a native FUN6 project.");
                uint hash=0x811c9dc5;foreach(byte value in o.Data.AsSpan(0,o.Data.Length-4))hash=unchecked((hash^value)*16777619);
                if(hash!=BinaryPrimitives.ReadUInt32LittleEndian(o.Data.AsSpan(o.Data.Length-4)))throw new IOException("Native project checksum is invalid.");
            }
            if(o.Id==1)_=Order(o.Data);
            if(o.Id>=32&&o.Data.Length!=0)_=SlotImages.Read(o.Data);
        }
        if(song.Patches.Select(p=>p.Track).Distinct().Count()!=song.Patches.Length)throw new IOException("Duplicate song patch.");
        foreach(var p in song.Patches){if(p.Track>2||p.Data is null||SlotBuilder.Crc32(p.Data)!=p.Crc)throw new IOException("Invalid song patch or checksum.");_=PatchCodec.Unpack(p.Data);}
    }
    static byte[] Order(byte[] settings)
    {
        if(settings.Length<84||BinaryPrimitives.ReadUInt32LittleEndian(settings)!=0x50455233)throw new IOException("Unsupported song-order settings.");
        var order=settings[48..84];if(order[0] is <1 or >16||order[1]>1)throw new IOException("Invalid song order.");
        for(int i=0;i<order[0];i++)if(order[4+i*2]>3||order[5+i*2] is <1 or >64)throw new IOException("Invalid song section.");
        return order;
    }
    static ulong ReadNumber(ReadOnlySpan<byte> bytes){ulong n=0;for(int i=0;i<bytes.Length;i++)n|=(ulong)bytes[i]<<(7*i);return n;}
    public static async Task<SongObject[]> CaptureObjects(EditorClient client,CancellationToken token)
    {
        var a=(await client.RequestAsync(34,[],timeout:TimeSpan.FromSeconds(10),cancellationToken:token)).Arguments;
        if(a.Length<2||a[0]!=0||a.Length!=2+a[1]*11)throw new IOException("Invalid backup inventory.");
        var objects=new List<SongObject>();var seen=new HashSet<byte>();
        for(int i=2;i<a.Length;i+=11) {
            byte id=a[i];if(!Required.Contains(id)&&id!=10)continue;
            if(!seen.Add(id))throw new IOException("Duplicate backup object.");
            int size=checked((int)ReadNumber(a.AsSpan(i+1,5)));uint crc=checked((uint)ReadNumber(a.AsSpan(i+6,5)));
            if(size<0||size>(id>=32?SlotBuilder.SlotBytes:4096))throw new IOException("Invalid backup size.");
            var bytes=new byte[size];
            for(int off=0;off<size;off+=256) {
                int count=Math.Min(256,size-off);var b=(await client.RequestAsync(35,[id,..Fm1Operations.Number((ulong)off),..Fm1Operations.Number((ulong)count,2)],cancellationToken:token)).Arguments;
                if(b.Length<9||b[0]!=id||b[1]!=0||ReadNumber(b.AsSpan(2,5))!=(ulong)off||ReadNumber(b.AsSpan(7,2))!=(ulong)count)throw new IOException("Song backup read failed.");
                var data=EditorCodec.Unpack7(b.AsSpan(9));if(data.Length!=count)throw new IOException("Truncated song backup.");data.CopyTo(bytes,off);
            }
            if(SlotBuilder.Crc32(bytes)!=crc)throw new IOException("Song backup checksum differs.");objects.Add(new(id,bytes,crc));
        }
        if(Required.Any(id=>!seen.Contains(id)))throw new IOException("Firmware is missing required song backup objects.");return objects.ToArray();
    }
    public static async Task<Fm1Song> Capture(EditorClient client,DeviceInfo info,string name,CancellationToken token)
    {
        Require(info);name=name.Trim();if(name.Length is <1 or >120)throw new ArgumentException("Use a song name between 1 and 120 characters.");
        var objects=await CaptureObjects(client,token);var patches=new List<SongPatch>();int fm6=Array.IndexOf(info.Engines.ToArray(),"FM6");
        for(byte track=0;track<3;track++) {
            var r=(await client.RequestAsync(29,[track],cancellationToken:token)).Arguments;
            if(r.Length!=3+info.ParameterCount*2||r[0]!=track)throw new IOException("Invalid song track reply.");
            if(r[1]==fm6){var packed=PatchCodec.Pack(await Fm1Operations.ReadPatch(client,track,token));patches.Add(new(track,packed,SlotBuilder.Crc32(packed)));}
        }
        var song=new Fm1Song(Guid.NewGuid(),name,info.Firmware,DateTimeOffset.UtcNow,info.ParameterCount,info.Engines.ToArray(),objects,patches.ToArray());Validate(song);return song;
    }
    static void Require(DeviceInfo info){if(info.ProtocolVersion<14||!info.HasSupportedTrackLayout||info.ParameterCount!=71)throw new IOException("Song library requires the supported SLOOP 2.6 firmware layout.");}
    static async Task Put(EditorClient client,byte id,byte[] data,CancellationToken token)
    {
        async Task Send(byte op,byte[] args){var a=(await client.RequestAsync(36,[op,id,..args],timeout:TimeSpan.FromSeconds(10),cancellationToken:token)).Arguments;
            if(a.Length!=3||a[0]!=op||a[1]!=id||a[2]!=0)throw new IOException($"Song restore failed on object {id}, stage {op}.");}
        await Send(0,[..Fm1Operations.Number((ulong)data.Length),..Fm1Operations.Number(SlotBuilder.Crc32(data))]);
        for(int off=0;off<data.Length;off+=256)await Send(1,[..Fm1Operations.Number((ulong)off),..EditorCodec.Pack7(data.AsSpan(off,Math.Min(256,data.Length-off)))]);
        await Send(2,[]);
    }
    public static async Task Restore(EditorClient client,DeviceInfo info,Fm1Song song,Func<Fm1Song,Task> retainBackup,CancellationToken token)
    {
        Validate(song);Require(info);if(!song.Engines.SequenceEqual(info.Engines))throw new IOException("Song engine layout differs from this firmware.");
        // A durable recovery archive must exist before the first device mutation.
        var before=await Capture(client,info,"Before loading "+song.Name[..Math.Min(song.Name.Length,90)],token);await retainBackup(before);token.ThrowIfCancellationRequested();
        var current=before.Objects.ToDictionary(o=>o.Id);var expected=song.Objects.Where(o=>o.Id!=10).ToDictionary(o=>o.Id,o=>o.Data);
        var settings=current[1].Data.ToArray();Order(song.Objects.Single(o=>o.Id==1).Data).CopyTo(settings,48);expected[1]=settings;
        foreach(byte id in new byte[]{32,33,34,35,6,7,8,9,2,3,4,5,1,0}) {
            var data=expected[id];if(current[id].Data.AsSpan().SequenceEqual(data))continue;
            if(id>=32) {
                byte slot=(byte)(id-32);
                if(data.Length!=0)await Fm1Operations.UploadSlot(client,slot,SlotImages.Read(data),_=>{},token);
                else {var a=(await client.RequestAsync(14,[slot],timeout:TimeSpan.FromSeconds(10),cancellationToken:token)).Arguments;if(a.Length!=2||a[0]!=slot||a[1]!=0)throw new IOException("Could not clear sample slot.");}
            } else await Put(client,id,data,token);
        }
        foreach(var p in song.Patches)await Fm1Operations.WritePatch(client,p.Track,PatchCodec.Unpack(p.Data),token);
        var actual=await CaptureObjects(client,token);
        foreach(var o in actual.Where(o=>expected.ContainsKey(o.Id))) {
            var wanted=expected[o.Id];bool equal=o.Id>=32&&wanted.Length!=0?
                o.Data.Length==wanted.Length&&o.Data.AsSpan(0,480).SequenceEqual(wanted.AsSpan(0,480))&&o.Data.AsSpan(512).SequenceEqual(wanted.AsSpan(512)):
                o.Data.AsSpan().SequenceEqual(wanted);
            if(!equal)throw new IOException($"Song readback differs on object {o.Id}. Recovery copy retained; reconnect before retrying.");
        }
    }
}

public sealed class Fm1SongStore(string directory)
{
    public void Save(Fm1Song song)=>WorkspaceFiles.StoreBytes(Path.Combine(directory,song.Id.ToString("N")+".fm1song"),Fm1Songs.Encode(song));
    public Fm1Song Read(Guid id){using var stream=File.OpenRead(Path.Combine(directory,id.ToString("N")+".fm1song"));return Fm1Songs.Decode(stream);}
    public IReadOnlyList<Fm1Song> List()=>!Directory.Exists(directory)?[]:Directory.GetFiles(directory,"*.fm1song").Select(path=>{using var stream=File.OpenRead(path);return Fm1Songs.Decode(stream);}).OrderByDescending(s=>s.CreatedUtc).ToArray();
}
