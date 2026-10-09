using Sloop.Protocol;
int checks = 0;
void Check(bool ok, string message) { checks++; if (!ok) throw new Exception(message); }
void Reject(byte[] data) {
    try { EditorClient.DecodeDrumGrooves(new(74, data)); throw new Exception("Malformed bank accepted."); }
    catch (FormatException) { checks++; }
}
byte[] list = [1,0,2,0,16,2,70,76,79,79,82,0,1,12,4,83,72,85,70,70,76,69,0];
var bank = EditorClient.DecodeDrumGrooves(new(74, list));
Check(bank.Count == 2 && bank[0] == new DrumGroove(0,"FLOOR",16,2) && bank[1].Name == "SHUFFLE", "Device bank decoding");
var fullList = new List<byte> { 1, 0, 24 };
for (byte id = 0; id < 24; id++) {
    fullList.AddRange([id, (byte)(id == 14 ? 64 : 16), 2]);
    fullList.AddRange(System.Text.Encoding.ASCII.GetBytes(id == 14 ? "AMEN BREAK" : $"GROOVE {id}"));
    fullList.Add(0);
}
var fullBank = EditorClient.DecodeDrumGrooves(new(74, fullList.ToArray()));
Check(fullBank.Count == 24 && fullBank[14] == new DrumGroove(14,"AMEN BREAK",64,2), "Full bank with four-bar Amen");
for (int i = 0; i < list.Length; i++) Reject(list[..i]);
Reject([..list,0]); Reject([1,0,0]); Reject([1,0,25]);
foreach (int offset in new[]{3,4,5,12,13,14}) {
    var corrupt = list.ToArray(); corrupt[offset] = 127; Reject(corrupt);
}
var wire = new Wire(); using var client = new EditorClient(wire);
DeviceInfo Info(int v) => new("SLOOP",1,1,1,64,0,["FM"],4,v);
try { await client.ListDrumGroovesAsync(Info(11)); throw new Exception("Old firmware probed"); } catch (NotSupportedException) { checks++; }
Check(wire.Sent == 0, "No unknown command on old firmware");
wire.Replies.Enqueue([0,0,1,2,1]); wire.Replies.Enqueue(list);
Check((await client.ListDrumGroovesAsync(Info(12))).Count == 2 && wire.Sent == 2, "Discover and list actual firmware bank");
for (byte status = 0; status <= 3; status++) {
    wire.Replies.Enqueue([2,status,1]);
    Check((byte)await client.ApplyDrumGrooveAsync(1,status == 0) == status, "Apply refusal/success is preserved");
    Check(wire.Last.SequenceEqual(new byte[]{2,1,(byte)(status == 0 ? 1 : 0)}), "Replacement confirmation travels on wire");
}
wire.Replies.Enqueue([0,0,1,24,1]); wire.Replies.Enqueue(fullList.ToArray());
Check((await client.ListDrumGroovesAsync(Info(13))).Count == 24, "Expanded native bank capability discovery");
wire.Replies.Enqueue([2,0,23]);
Check(await client.ApplyDrumGrooveAsync(23,true) == DrumGrooveStatus.Applied, "Expanded bank last ID applies");
wire.Replies.Enqueue([2,0,0]);
try { await client.ApplyDrumGrooveAsync(1,true); throw new Exception("Wrong groove ack accepted"); } catch (FormatException) { checks++; }
int sent = wire.Sent;
try { await client.ApplyDrumGrooveAsync(24,true); throw new Exception("Invalid groove sent"); } catch (ArgumentOutOfRangeException) { checks++; }
Check(wire.Sent == sent, "Invalid IDs rejected before send");
Console.WriteLine($"Drum groove protocol: {checks} checks passed.");

sealed class Wire : IMidiTransport
{
    public event Action<ReadOnlyMemory<byte>>? BytesReceived;
    public event Action? Disconnected { add { } remove { } }
    public Queue<byte[]> Replies { get; } = new();
    public int Sent; public byte[] Last = [];
    public ValueTask SendAsync(ReadOnlyMemory<byte> bytes, CancellationToken token) {
        token.ThrowIfCancellationRequested(); Sent++; Last = bytes.Span[5..^1].ToArray();
        BytesReceived?.Invoke(EditorCodec.Encode(74, Replies.Dequeue())); return ValueTask.CompletedTask;
    }
}
