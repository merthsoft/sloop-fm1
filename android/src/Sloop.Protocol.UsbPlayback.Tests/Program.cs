using Sloop.Protocol;
static void Check(bool value,string message) { if(!value) throw new Exception(message); }
var transport=new Wire();using var client=new EditorClient(transport);
DeviceInfo Info(int v)=>new("test",1,1,1,16,0,["FM"],4,v);
Check(!(await client.GetUsbPlaybackCapabilitiesAsync(Info(11))).Supported && transport.Sent==0,"old device must not be probed");
transport.Reply=[0,0,1,0,0,32,0,2];
Check(!(await client.GetUsbPlaybackCapabilitiesAsync(Info(12))).Supported,"UAC absent capabilities");
transport.Reply=[0,0,1,15,0,32,0,2];var caps=await client.GetUsbPlaybackCapabilitiesAsync(Info(12));
Check(caps.Supported && caps.RampFrames==256,"capabilities");
transport.Reply=[1,0,0,16,1];var state=await client.UsbPlaybackControlAsync(caps,new(2048,true));
Check(state==new UsbPlaybackControl(2048,true) && transport.Last.SequenceEqual(new byte[]{1,0,16,1}),"gain/mute wire round trip");
int sent=transport.Sent;try { await client.UsbPlaybackControlAsync(caps,new(4097,false));throw new Exception("accepted gain"); } catch(ArgumentOutOfRangeException) {}
Check(transport.Sent==sent,"invalid gain sent");
transport.Reply=[1,0,1,32,0];try { await client.UsbPlaybackControlAsync(caps);throw new Exception("accepted malformed state"); } catch(FormatException) {}
var diag=new byte[63];diag[0]=2;diag[2]=127;diag[3]=127;diag[4]=127;diag[5]=127;diag[6]=15;
void U32(int index,uint value) { for(int j=0;j<5;j++)diag[2+index*5+j]=(byte)((value>>(7*j))&127); }
U32(8,1024);U32(11,65510);diag[62]=1;transport.Reply=diag;
var d=await client.GetUsbPlaybackDiagnosticsAsync(caps);Check(d.Packets==uint.MaxValue && d.AlternateSelected && d.RateQ16==65510,"uint32 diagnostics");
diag[0]=3;U32(0,0);transport.Reply=diag;d=await client.GetUsbPlaybackDiagnosticsAsync(caps,true);Check(d.Packets==0 && transport.Last.SequenceEqual(new byte[]{3}),"reset subop");
diag[0]=2;diag[6]=16;transport.Reply=diag;try { await client.GetUsbPlaybackDiagnosticsAsync(caps);throw new Exception("accepted overflow"); } catch(FormatException) {}
transport.Reply=[1,2];try { await client.UsbPlaybackControlAsync(caps);throw new Exception("accepted unavailable"); } catch(NotSupportedException) {}
Console.WriteLine("USB playback protocol: PASS (capabilities, unsupported devices, control wire, malformed gain/state/counters, snapshots/reset)");
sealed class Wire:IMidiTransport
{
    public event Action<ReadOnlyMemory<byte>>? BytesReceived;
    public event Action? Disconnected { add {} remove {} }
    public byte[] Reply=[]; public byte[] Last=[];public int Sent;
    public ValueTask SendAsync(ReadOnlyMemory<byte> bytes,CancellationToken token) {
        token.ThrowIfCancellationRequested();Sent++;Last=bytes.Span[5..^1].ToArray();
        BytesReceived?.Invoke(EditorCodec.Encode(75,Reply));return ValueTask.CompletedTask;
    }
}
