using Sloop.Protocol;
int checks=0;
void Check(bool value,string name){checks++;if(!value)throw new Exception(name);}
void Reject(byte[] a){try{EditorClient.DecodeMusicalStarterNames(new(76,a),1);throw new Exception("Accepted malformed bank");}catch(FormatException){checks++;}}
var list=new List<byte>{1,0,24};for(byte i=0;i<24;i++){list.Add(i);list.AddRange(System.Text.Encoding.ASCII.GetBytes($"STARTER {i}"));list.Add(0);}
var bank=EditorClient.DecodeMusicalStarterNames(new(76,list.ToArray()),1);Check(bank.Count==24&&bank[23].Id==23,"full device bank");
for(int i=0;i<list.Count;i++)Reject(list.Take(i).ToArray());Reject([..list,0]);var bad=list.ToArray();bad[3]=1;Reject(bad);
var options=new MusicalStarterOptions(2,23,11,16,-3,2,true,-16,8,2,31);
Check(options.Encode(2,true).SequenceEqual(new byte[]{2,2,23,11,16,0,2,1,0,16,2,63,1}),"exact offset wire values");
foreach(var o in new[]{options with{Track=3},options with{Id=24},options with{Octave=4},options with{Feel=-33},options with{Feel=32},options with{Rotate=17},options with{Offset=-9},options with{Mode=3},options with{Syncopation=3}})
    try{o.Encode(2);throw new Exception("Invalid options accepted");}catch(ArgumentOutOfRangeException){checks++;}
using var wire=new Wire();using var client=new EditorClient(wire);
DeviceInfo Info(int v)=>new("SLOOP",1,1,1,64,0,["FM"],4,v);
try{await client.ListMusicalStartersAsync(Info(13));throw new Exception("Old device probed");}catch(NotSupportedException){checks++;}Check(wire.Sent==0,"old device no unknown commands");
wire.Replies.Enqueue([0,0,1,24,7,2,92,11]);wire.Replies.Enqueue(list.ToArray());wire.Replies.Enqueue([5,0,2,0,67,72,82,0,1,77,65,74,0]);
var discovered=await client.ListMusicalStartersAsync(Info(14));Check(discovered.Scales[1].Name=="MAJ"&&discovered.LeaseMilliseconds==1500,"actual capabilities and scale names");
for(byte s=0;s<=4;s++){wire.Replies.Enqueue([2,s]);Check((byte)await client.ApplyMusicalStarterAsync(options,true)==s,"apply statuses");}
wire.Replies.Enqueue([3,0]);await client.PreviewMusicalStarterAsync(options,513);Check(wire.Last[^2]==1&&wire.Last[^1]==4,"preview token encoding");
wire.Replies.Enqueue([4,0]);await client.MusicalStarterLeaseAsync(513,true);Check(wire.Last.SequenceEqual(new byte[]{4,1,4,1}),"lease renewal");
wire.Replies.Enqueue([4,4]);Check(await client.MusicalStarterLeaseAsync(513,false)==MusicalStarterStatus.Stale,"stale preview cannot stop another owner");
wire.Replies.Enqueue([2,0,99]);try{await client.ApplyMusicalStarterAsync(options,true);throw new Exception("Malformed ack accepted");}catch(FormatException){checks++;}
Console.WriteLine($"Musical starter protocol: {checks} checks passed.");
sealed class Wire:IMidiTransport,IDisposable {
    public event Action<ReadOnlyMemory<byte>>? BytesReceived;
    public event Action? Disconnected{add{}remove{}}
    public Queue<byte[]> Replies=new();public int Sent;public byte[] Last=[];
    public ValueTask SendAsync(ReadOnlyMemory<byte> data,CancellationToken t){t.ThrowIfCancellationRequested();Sent++;Last=data.Span[5..^1].ToArray();BytesReceived?.Invoke(EditorCodec.Encode(76,Replies.Dequeue()));return ValueTask.CompletedTask;}
    public void Dispose(){}
}
