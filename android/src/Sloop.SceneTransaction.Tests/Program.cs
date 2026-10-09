using Sloop.Protocol;

int checks=0;
void Check(bool value) { checks++; if(!value) throw new Exception($"Check {checks} failed."); }
void Reject(Action action) { checks++; try { action(); } catch(ArgumentException) { return; } catch(FormatException) { return; } catch(NotSupportedException) { return; } throw new Exception("Expected rejection."); }
byte[] Track()
{
    var p=new byte[SceneTransactionCodec.TrackBytes]; p[142]=64; p[143]=2;
    for(int i=0;i<64;i++) { p[144+i*11+8]=255; p[144+i*11+9]=255; p[144+i*11+10]=32; }
    for(int i=0;i<24;i++) p[864+i*5]=255;
    return p;
}
var tracks=new[]{Track(),Track(),Track()}; var payload=SceneTransactionCodec.Build(120,tracks);
Check(payload.Length==2956); Check(SceneTransactionCodec.Crc("123456789"u8)==0xcbf43926);
Check(EditorCodec.Unpack7(EditorCodec.Pack7(payload)).SequenceEqual(payload));
Check(SceneTransactionCodec.Build(300,tracks).Length<=3072);
Reject(()=>SceneTransactionCodec.Build(19,tracks)); Reject(()=>SceneTransactionCodec.Build(301,tracks));
Reject(()=>SceneTransactionCodec.Build(120,tracks[..2]));
Reject(()=>SceneTransactionCodec.Build(120,tracks,[new("asset",SceneSampleDependencyState.Missing)]));
Reject(()=>SceneTransactionCodec.Build(120,tracks,[new("asset",SceneSampleDependencyState.Unsupported)]));
Check(SceneTransactionCodec.Build(120,tracks,[new("asset",SceneSampleDependencyState.VerifiedFlashResident)]).Length==2956);
tracks[0][145]=128; Reject(()=>SceneTransactionCodec.Build(120,tracks)); tracks[0]=Track();
tracks[0][143]=9; Reject(()=>SceneTransactionCodec.Build(120,tracks)); tracks[0]=Track();
tracks[0][848]=255; Reject(()=>SceneTransactionCodec.Build(120,tracks)); tracks[0]=Track();
tracks[0][864]=64; Reject(()=>SceneTransactionCodec.Build(120,tracks)); tracks[0]=Track();
var transport=new FakeTransport(); using var editor=new EditorClient(transport); var scene=new SceneTransactionClient(editor);
try { await scene.CapabilitiesAsync(10); throw new Exception("Old protocol accepted."); } catch(NotSupportedException) { checks++; }
var caps=await scene.CapabilitiesAsync(11); Check(caps.SupportsAtomicScene);
var prepared=await scene.StageAsync(caps,1,42,payload); Check(prepared.State==SceneTransactionState.Prepared);
Check(transport.Bytes.SequenceEqual(payload)); Check(transport.Frames==34); Check(transport.MaximumWire<=125);
var queued=await scene.CommitAsync(prepared,100,SceneMusicalBoundary.Bar); Check(queued.State==SceneTransactionState.Queued);
Check((await scene.StatusAsync(1,1)).State==SceneTransactionState.Queued);
Check((await scene.CancelAsync(1,1)).State==SceneTransactionState.Canceled);
// Existing request client's ambiguous failure policy must remain effective.
transport.Drop=true;
try { await editor.RequestAsync(72,[0],timeout:TimeSpan.FromMilliseconds(10)); throw new Exception("Missing timeout."); } catch(TimeoutException) { checks++; }
try { await scene.StatusAsync(1,1); throw new Exception("Faulted epoch reused."); } catch(IOException) { checks++; }
Console.WriteLine($"Scene transaction host: {checks} checks passed; {payload.Length} raw bytes, {transport.MaximumWire} max wire bytes.");

sealed class FakeTransport : IMidiTransport
{
    public event Action<ReadOnlyMemory<byte>>? BytesReceived;
    public event Action? Disconnected { add { } remove { } }
    public List<byte> Bytes { get; }=[];
    public int Frames,MaximumWire;
    public bool Drop;
    byte state;
    public ValueTask SendAsync(ReadOnlyMemory<byte> memory,CancellationToken token)
    {
        if(Drop) return ValueTask.CompletedTask;
        var wire=memory.ToArray(); MaximumWire=Math.Max(MaximumWire,wire.Length); Frames++;
        var a=wire.AsSpan(5,wire.Length-6); byte op=a[0];
        if(op==1) state=1;
        if(op==2) { int off=a[7]|a[8]<<7; if(off!=Bytes.Count) throw new Exception("Non-contiguous chunk."); Bytes.AddRange(EditorCodec.Unpack7(a[9..])); }
        if(op==3) state=2; if(op==4) state=3; if(op==5) state=5;
        var p=new byte[24]; p[0]=op; p[2]=state; p[3]=1; p[7]=op==0?(byte)0:a[5]; p[8]=op==0?(byte)0:a[6];
        p[9]=(byte)(Bytes.Count&127); p[10]=(byte)(Bytes.Count>>7); p[11]=42;
        if(op==4) { a.Slice(7,4).CopyTo(p.AsSpan(15)); p[19]=a[11]; }
        p[20]=0; p[21]=24; p[22]=96; p[23]=7;
        BytesReceived?.Invoke(EditorCodec.Encode(72,p)); return ValueTask.CompletedTask;
    }
}
