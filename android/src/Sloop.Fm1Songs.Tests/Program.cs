using System.Buffers.Binary;
using Sloop.Protocol;
using Sloop.SampleEncoding;
using Sloop.Workstation;
using Sloop.SoundDesign;

int checks=0;
void Check(bool ok,string message){if(!ok)throw new Exception(message);checks++;}
async Task Reject(Func<Task> action,string message){try{await action();}catch(IOException){checks++;return;}throw new Exception(message);}
var info=new DeviceInfo("2.6 test",2,71,45,64,63,new[]{"FM6","SAMPLE"},4,14);
var wire=new SongWire();using var client=new EditorClient(wire);
var song=await Fm1Songs.Capture(client,info,"Test song",default);
Check(song.Objects.Length==15&&song.Objects.Single(o=>o.Id==0).Data.Length==3816,"Capture includes working project, sections, banks, samples and optional MIDI backup.");
Check(song.Patches.Length==3,"Edited FM6 track voices captured separately from bank selectors.");
using(var input=new MemoryStream(Fm1Songs.Encode(song)))Check(Fm1Songs.Decode(input).Objects.Single(o=>o.Id==0).Crc==song.Objects.Single(o=>o.Id==0).Crc,"Archive round-trip retains verified bytes.");
var canonical=File.ReadAllBytes(Path.Combine(AppContext.BaseDirectory,"upstream271.fun6"));
var upstreamSong=song with {Firmware="SLOOP 2.7.1",Objects=song.Objects.Select(o=>o.Id==0?new SongObject(0,canonical,SlotBuilder.Crc32(canonical)):o).ToArray()};
using(var input=new MemoryStream(Fm1Songs.Encode(upstreamSong)))Check(Fm1Songs.Decode(input).Objects.Single(o=>o.Id==0).Data.SequenceEqual(canonical),"Genuine upstream FUN6 continuations, arp IDs and drum settings survive the phone library unchanged.");
// Optional firmware metadata uses signed action words; the library must keep raw bytes.
for(int mode=1;mode<=2;mode++){
 var custom=canonical.ToArray();BinaryPrimitives.WriteUInt16LittleEndian(custom.AsSpan(8+20*2),(ushort)(0xED00|mode));
 BinaryPrimitives.WriteUInt16LittleEndian(custom.AsSpan(8+22*2),0xD44F);
 BinaryPrimitives.WriteUInt16LittleEndian(custom.AsSpan(8+23*2),0xD843);
 BinaryPrimitives.WriteUInt16LittleEndian(custom.AsSpan(8+31*2),(ushort)(0x8000|20|(8<<5)|(4<<10)));SongWire.Seal(custom);
 var customSong=upstreamSong with{Objects=upstreamSong.Objects.Select(o=>o.Id==0?new SongObject(0,custom,SlotBuilder.Crc32(custom)):o).ToArray()};
 using var input=new MemoryStream(Fm1Songs.Encode(customSong));
 Check(Fm1Songs.Decode(input).Objects.Single(o=>o.Id==0).Data.SequenceEqual(custom),"Delay character metadata survives the phone song archive byte-for-byte.");
}
var bad=song with{Objects=song.Objects.Select(o=>o.Id==0?o with{Crc=0}:o).ToArray()};
await Reject(()=>Fm1Songs.Restore(client,info,bad,_=>Task.CompletedTask,default),"Corrupt archive is rejected before touching device.");
Check(wire.Writes.Count==0,"Invalid archive caused no writes.");
await Reject(()=>Fm1Songs.Restore(client,info,song with{Patches=song.Patches.Select(p=>p with{Crc=0}).ToArray()},_=>Task.CompletedTask,default),"Edited FM6 patch corruption rejected before writes.");
await Reject(()=>Fm1Songs.Restore(client,info,song with{Engines=["wrong"]},_=>Task.CompletedTask,default),"Engine layout mismatch rejected.");
await Reject(()=>Fm1Songs.Restore(client,info,song,_=>throw new IOException("Disk full"),default),"Failed durable backup stops restore.");
Check(wire.Writes.Count==0,"Disk-full backup failure caused no writes.");
// Target changes a section, working project, bank, sample and arrangement.
var objects=song.Objects.Select(o=>{
    var b=o.Data.ToArray();
    if(o.Id==0)b[100]=7;
    if(o.Id==2){b=new byte[3816];BinaryPrimitives.WriteUInt32LittleEndian(b,0x46554e36);b[100]=9;}
    if(o.Id==6)b=[3,4,5];
    if(o.Id==1)b[53]=8;
    if(o.Id==32)b=SlotBuilder.Build("TEST",new[]{new ZoneInput(new short[32],60,0,127)}).Image;
    if(o.Id==0||o.Id==2)SongWire.Seal(b);
    return new SongObject(o.Id,b,SlotBuilder.Crc32(b));
}).ToArray();var target=song with{Objects=objects};bool backed=false;
await Fm1Songs.Restore(client,info,target,s=>{Check(s.Objects.Single(o=>o.Id==0).Data[100]==0,"Recovery archive contains pre-load working project.");backed=true;wire.BackupDone=true;return Task.CompletedTask;},default);
Check(backed&&wire.Writes.All(w=>w.BackupDone),"Every mutation follows retained recovery archive.");
Check(wire.Objects[0][100]==7&&wire.Objects[2][100]==9,"Working project and A–D section restored.");
Check(wire.Objects[1][53]==8&&wire.Objects[1][12]==99,"Song order restored while device preferences retained.");
Check(wire.Objects[10].SequenceEqual(new byte[]{42}),"MIDI configuration not overwritten.");
Check(wire.Writes.FindIndex(w=>w.Id==32)<wire.Writes.FindIndex(w=>w.Id==0)&&wire.Writes.FindIndex(w=>w.Id==6)<wire.Writes.FindIndex(w=>w.Id==0),"Sound dependencies committed before working project.");
Check(wire.Writes.Count(w=>w.Id==69)==3,"Live edited FM6 voices reapplied after project load and individually read back.");
int previousWrites=wire.Writes.Count;wire.CorruptReadback=true;
await Reject(()=>Fm1Songs.Restore(client,info,target,_=>Task.CompletedTask,default),"Corrupt snapshot readback prevents backup and restore.");wire.CorruptReadback=false;
Check(wire.Writes.Count==previousWrites,"Corrupt recovery capture prevented writes.");
wire.CorruptAfterWrite=true;
await Reject(()=>Fm1Songs.Restore(client,info,target,_=>Task.CompletedTask,default),"Final device readback failure prevents a false success.");wire.CorruptAfterWrite=false;wire.CorruptReadback=false;
await Reject(()=>Fm1Songs.Restore(client,info with{ProtocolVersion=13},target,_=>Task.CompletedTask,default),"Unsupported firmware rejected before capture.");
await Reject(()=>Fm1Songs.Restore(client,info,target with{Objects=[..objects,objects[0]]},_=>Task.CompletedTask,default),"Duplicate archive object rejected.");
var folder=Path.Combine(Path.GetTempPath(),"songs-"+Guid.NewGuid().ToString("N"));
try{var store=new Fm1SongStore(folder);store.Save(target);Check(store.List().Count==1&&store.Read(target.Id).Name=="Test song","Durable named song store lists and reopens archive.");}
finally{if(Directory.Exists(folder))Directory.Delete(folder,true);}
Console.WriteLine($"FM1 songs: {checks} archive, backup-before-write, dependency-order and restore checks passed.");

sealed class SongWire:IMidiTransport
{
    public event Action<ReadOnlyMemory<byte>>? BytesReceived;
    public event Action? Disconnected {add{}remove{}}
    public Dictionary<byte,byte[]> Objects=[];
    public List<(byte Id,bool BackupDone)> Writes=[];
    public bool BackupDone,CorruptReadback,CorruptAfterWrite;
    byte[] patch=PatchCodec.Pack(FactoryLibrary.Get(1));
    byte stagingId,sampleSlot;MemoryStream staging=new();byte[] sample=[];
    static ulong Num(ReadOnlySpan<byte> b){ulong n=0;for(int i=0;i<b.Length;i++)n|=(ulong)b[i]<<(7*i);return n;}
    public SongWire(){foreach(byte id in new byte[]{0,1,2,3,4,5,6,7,8,9,10,32,33,34,35})Objects[id]=[];
        Objects[0]=new byte[3816];BinaryPrimitives.WriteUInt32LittleEndian(Objects[0],0x46554e36);Seal(Objects[0]);
        Objects[1]=new byte[100];BinaryPrimitives.WriteUInt32LittleEndian(Objects[1],0x50455233);Objects[1][12]=99;Objects[1][48]=1;Objects[1][53]=4;Objects[10]=[42];}
    public ValueTask SendAsync(ReadOnlyMemory<byte> bytes,CancellationToken token){
        token.ThrowIfCancellationRequested();byte cmd=bytes.Span[4];var a=bytes.Span[5..^1].ToArray();byte[] r;
        switch(cmd){
            case 34:var inventory=new List<byte>{0,(byte)Objects.Count};foreach(var o in Objects){inventory.Add(o.Key);inventory.AddRange(Fm1Operations.Number((ulong)o.Value.Length));inventory.AddRange(Fm1Operations.Number(SlotBuilder.Crc32(o.Value)));}r=inventory.ToArray();break;
            case 35:byte id=a[0];int off=(int)Num(a.AsSpan(1,5)),count=(int)Num(a.AsSpan(6,2));var part=Objects[id].AsSpan(off,count).ToArray();if(CorruptReadback&&id==0&&off==0)part[0]^=1;r=[id,0,..Fm1Operations.Number((ulong)off),..Fm1Operations.Number((ulong)count,2),..EditorCodec.Pack7(part)];break;
            case 29:r=[a[0],0,0,..new byte[142]];break;
            case 68:r=[a[0],a[1],0,..patch];break;
            case 69:patch=a[2..];Writes.Add((69,BackupDone));if(CorruptAfterWrite)CorruptReadback=true;r=[a[0],a[1],0];break;
            case 36:byte op=a[0];id=a[1];if(op==0){stagingId=id;staging=new();}if(op==1)staging.Write(EditorCodec.Unpack7(a.AsSpan(7)));if(op==2){Objects[stagingId]=staging.ToArray();Writes.Add((id,BackupDone));}r=[op,id,0];break;
            case 11:sampleSlot=a[0];sample=new byte[512];r=[sampleSlot,0];break;
            case 12:off=(int)Num(a.AsSpan(1,3));part=EditorCodec.Unpack7(a.AsSpan(4));Array.Resize(ref sample,off+part.Length);part.CopyTo(sample,off);r=[a[0],..a.AsSpan(1,3).ToArray(),0];break;
            case 13:EditorCodec.Unpack7(a.AsSpan(1)).CopyTo(sample,0);Objects[(byte)(32+sampleSlot)]=sample;Writes.Add(((byte)(32+sampleSlot),BackupDone));r=[sampleSlot,0];break;
            case 14:Objects[(byte)(32+a[0])]=[];Writes.Add(((byte)(32+a[0]),BackupDone));r=[a[0],0];break;
            default:throw new IOException("Unexpected song command "+cmd);
        }
        BytesReceived?.Invoke(EditorCodec.Encode(cmd,r));return ValueTask.CompletedTask;
    }
    public static void Seal(byte[] bytes){BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(4),(uint)bytes.Length);uint h=0x811c9dc5;foreach(byte b in bytes.AsSpan(0,bytes.Length-4))h=unchecked((h^b)*16777619);BinaryPrimitives.WriteUInt32LittleEndian(bytes.AsSpan(bytes.Length-4),h);}
}
