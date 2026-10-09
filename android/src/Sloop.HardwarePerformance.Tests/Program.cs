using Sloop.Protocol;
int checks = 0;
void Check(bool value) { checks++; if (!value) throw new Exception($"Check {checks}"); }
void Bad(Action action) { try { action(); throw new Exception("Accepted malformed data"); } catch (ArgumentException) { checks++; } catch (FormatException) { checks++; } }
var caps = new PerformanceCapabilities(1, 7, 8, 750, 16);
Check(caps.Supported);
foreach (uint owner in new uint[] { 1, 127, 128, 16384, 0xfffffff }) {
    var bytes = PerformanceWire.Token(owner); Check(bytes.All(b => b <= 127));
    Check(PerformanceWire.ParseReply(new(73, [2, 0, .. bytes]), 2, owner) == PerformanceStatus.Ok);
}
Bad(() => PerformanceWire.Token(0)); Bad(() => PerformanceWire.Token(0x10000000));
for (int i = 0; i < 16; i++) Check(PerformanceWire.Press(1, HardwareGestureKind.Punch, (byte)i)[6] == i);
Bad(() => PerformanceWire.Press(1, HardwareGestureKind.Punch, 16));
Bad(() => PerformanceWire.Press(1, HardwareGestureKind.Fill, 1));
Bad(() => PerformanceWire.ParseReply(new(73, [2,0,1,0,0]), 2, 1));
Bad(() => PerformanceWire.ParseReply(new(73, [2,0,2,0,0,0]), 2, 1));
Bad(() => PerformanceWire.ParseCapabilities(new(73, [0,0,1,7,8,110,5,16,0])));
var midi = new FakeTransport(); using var client = new EditorClient(midi);
Check(await client.DiscoverPerformanceAsync(10) is null); Check(midi.Operations.Count == 0);
Check((await client.DiscoverPerformanceAsync(12))!.Supported);
var session = new HardwarePerformanceSession(client, caps);
midi.HoldPress = new(TaskCreationOptions.RunContinuationsAsynchronously);
var pressing = session.PressAsync(99, HardwareGestureKind.Fill);
var releasing = session.ReleaseAsync(99);
Check(!pressing.IsCompleted && !releasing.IsCompleted && midi.Operations.Last()[0] == 1);
midi.HoldPress.SetResult(); await pressing; await releasing; midi.HoldPress = null;
Check(midi.Operations.TakeLast(2).Select(a => a[0]).SequenceEqual(new byte[] {1,2}));
await session.PressAsync(1, HardwareGestureKind.Fill); await session.PressAsync(2, HardwareGestureKind.Punch, 5);
await session.ReleaseAsync(1); await session.RenewAsync(); Check(midi.Operations.Last()[0] == 3);
await session.ClearAsync(); Check(midi.Operations.Last()[0] == 4);
using var cancellation = new CancellationTokenSource();
midi.AfterPress = cancellation.Cancel;
try { await session.PressAsync(3, HardwareGestureKind.Fill, cancellation: cancellation.Token); throw new Exception("Cancellation ignored"); }
catch (OperationCanceledException) { checks++; }
Check(midi.Operations.TakeLast(2).Select(a => a[0]).SequenceEqual(new byte[] {1,2}));
midi.AfterPress = null;
await session.PressAsync(4, HardwareGestureKind.Fill); midi.Status = PerformanceStatus.Missing; await session.RenewAsync();
int count = midi.Operations.Count; await session.ReleaseAsync(4); Check(midi.Operations.Count == count);
midi.Status = PerformanceStatus.Ok;
await session.PressAsync(5, HardwareGestureKind.Fill); midi.Disconnect();
try { await session.RenewAsync(); throw new Exception("Disconnect ignored"); } catch (IOException) { checks++; }
Check(!session.Available);
Console.WriteLine($"Hardware performance: {checks} checks passed");
sealed class FakeTransport : IMidiTransport
{
    public event Action<ReadOnlyMemory<byte>>? BytesReceived;
    public event Action? Disconnected;
    public List<byte[]> Operations = [];
    public Action? AfterPress;
    public TaskCompletionSource? HoldPress;
    public PerformanceStatus Status;
    public void Disconnect() => Disconnected?.Invoke();
    public async ValueTask SendAsync(ReadOnlyMemory<byte> bytes, CancellationToken cancellationToken)
    {
        var a = bytes.ToArray()[5..^1]; Operations.Add(a);
        byte[] reply = a[0] == 0 ? [0,0,1,7,8,110,5,16] : [a[0],(byte)Status,.. a[0] == 4 ? new byte[4] : a[1..5]];
        if (a[0] == 1) AfterPress?.Invoke();
        if (a[0] == 1 && HoldPress is {} hold) await hold.Task;
        BytesReceived?.Invoke(EditorCodec.Encode(73, reply));
    }
}
