using System.Text;
using Sloop.Protocol;
using Sloop.Simulator;
using Sloop.Core.Sampling;

var passed = 0;
void Check(bool condition, string name)
{
    if (!condition) throw new Exception(name);
    passed++;
}
void Reject(Action action, string name)
{
    try { action(); } catch (FormatException) { passed++; return; }
    throw new Exception(name);
}

foreach (var value in new[] { -8192, -1, 0, 1, 8191 })
    Check(EditorCodec.DecodeValue(EditorCodec.EncodeValue(value)) == value, "signed value round trip");
Check(EditorCodec.Encode(1, []).SequenceEqual(new byte[] { 0xF0, 0x7D, 0x46, 0x4C, 1, 0xF7 }), "INFO wire vector");
Check(EditorCodec.Pack7([0x80, 0x01, 0xFF]).SequenceEqual(new byte[] { 5, 0, 1, 127 }), "pack7 mask vector");
for (var count = 0; count <= 256; count++)
{
    var bytes = Enumerable.Range(0, count).Select(i => (byte)(i * 31)).ToArray();
    Check(EditorCodec.Unpack7(EditorCodec.Pack7(bytes)).SequenceEqual(bytes), "pack7 round trip");
}
Reject(() => EditorCodec.Unpack7([1]), "orphan mask");
Reject(() => EditorCodec.Unpack7([2, 1]), "unused mask bit");
Reject(() => EditorCodec.DecodeValue([128, 0]), "non-seven-bit value");

var parser = new EditorStreamParser(8);
var frames = new List<EditorFrame>();
parser.FrameReceived += frames.Add;
parser.Feed([0x90, 60, 100, 0xF0, 0x7D]);
parser.Feed([0xF8, 0x46, 0x4C, 1, 42, 0xF7]);
Check(frames.Count == 1 && frames[0].Arguments.SequenceEqual(new byte[] { 42 }), "fragment and realtime");
parser.Feed([0xF0, 0x7D, 0x46, 0x4C, 1, 0x90, 0xF7]);
parser.Feed([0xF0, 1, 2, 3, 4, 0xF7]);
parser.Feed([0xF0, 0x7D, 0x46, 0x4C, 1, 1, 2, 3, 4, 5, 0xF7]);
Check(frames.Count == 1, "aborted, foreign and oversized frames ignored");
parser.Feed([0xF0, 1, 0xF0, 0x7D, 0x46, 0x4C, 25, 0, 0xF7]);
Check(frames.Count == 2 && frames[1].Command == 25, "restart after malformed frame");

byte[] Info(bool modern) => [.. Encoding.ASCII.GetBytes("FELUCCA test"), 0, 1, 58, 32, 64, 50,
    .. Encoding.ASCII.GetBytes("ANALOG"), 0, .. (modern ? new byte[] { 4, 9 } : Array.Empty<byte>())];
var info = DeviceInfo.Parse(new(1, Info(true)));
Check(info.TrackCount == 4 && info.ProtocolVersion == 9 && info.Engines[0] == "ANALOG", "v9 INFO");
Check(DeviceInfo.Parse(new(1, Info(false))).TrackCount == 1, "legacy INFO");
Reject(() => DeviceInfo.Parse(new(1, [65, 66])), "truncated INFO");
var descriptor = ParameterDescriptor.Parse(new(5, [0, 0, 3, .. EditorCodec.EncodeValue(0),
    .. EditorCodec.EncodeValue(127), .. EditorCodec.EncodeValue(104), 76, 86, 76, 0, 0]));
Check(descriptor.Label == "LVL" && descriptor.Maximum == 127 && descriptor.Default == 104, "DESC vector");
Reject(() => ParameterDescriptor.Parse(new(5, [0, 0, 3, .. EditorCodec.EncodeValue(127),
    .. EditorCodec.EncodeValue(0), .. EditorCodec.EncodeValue(104), 0, 0])), "invalid descriptor range");

var transport = new FakeTransport();
using var client = new EditorClient(transport);
var pushes = 0;
client.NotificationReceived += _ => pushes++;
var request = client.RequestAsync(2, [0, 1], f => f.Arguments.Length == 4 && f.Arguments[1] == 1);
transport.Receive(EditorCodec.Encode(23, [0, 2, 0, 64]));
transport.Receive(EditorCodec.Encode(2, [0, 3, 0, 64]));
Check(!request.IsCompleted, "wrong echoed ID ignored");
transport.Receive(EditorCodec.Encode(2, [0, 1, 0, 64]));
Check((await request).Command == 2 && pushes == 1, "push during request");

var first = client.RequestAsync(25, []);
var second = client.RequestAsync(1, []);
Check(transport.Sent.Count == 2, "requests serialized");
transport.Receive(EditorCodec.Encode(25, [0]));
await first;
await transport.WaitForSends(3);
transport.Receive(EditorCodec.Encode(1, Info(true)));
await second;
Check(transport.Sent.Count == 3, "queued request sent after reply");

var appliedLevel = client.TrackParameterAsync(2, 0, 127);
Check(transport.Sent[^1].SequenceEqual(EditorCodec.Encode(31, [2, 0, .. EditorCodec.EncodeValue(127)])), "level SET wire vector");
transport.Receive(EditorCodec.Encode(31, [1, 0, .. EditorCodec.EncodeValue(100)]));
Check(!appliedLevel.IsCompleted, "wrong track ignored");
transport.Receive(EditorCodec.Encode(31, [2, 0, .. EditorCodec.EncodeValue(126)]));
Check(await appliedLevel == 126, "acknowledged clamped value returned");

using (var canceled = new CancellationTokenSource())
{
    canceled.Cancel();
    try { await client.RequestAsync(25, [], cancellationToken: canceled.Token); throw new Exception("canceled request sent"); }
    catch (OperationCanceledException) { passed++; }
}
Check(transport.Sent.Count == 4, "preflight cancellation does not send or fault link");

try { await client.RequestAsync(25, [], timeout: TimeSpan.FromMilliseconds(20)); throw new Exception("timeout missing"); }
catch (TimeoutException) { passed++; }
try { await client.RequestAsync(25, []); throw new Exception("faulted session reused"); }
catch (IOException) { passed++; }
Check(transport.Sent.Count == 5, "fault prevents another send");

var unplugged = new FakeTransport();
{
    var octaveTransport = new FakeTransport();
    using var octaveClient = new EditorClient(octaveTransport);
    for(int offset=-3;offset<=3;offset++) {
        var reading = octaveClient.GetHardwareOctaveAsync();
        octaveTransport.Receive(EditorCodec.Encode(43,EditorCodec.EncodeValue(offset)));
        Check(await reading==offset,"signed hardware octave decoded");
    }
    Check(octaveTransport.Sent.All(w=>w.SequenceEqual(EditorCodec.Encode(43,[]))),"performance state is a read-only empty-argument request");
    foreach(var replyArgs in new byte[][] { [1], EditorCodec.EncodeValue(4) }) {
        var reading=octaveClient.GetHardwareOctaveAsync();
        octaveTransport.Receive(EditorCodec.Encode(43,replyArgs));
        try { await reading; throw new Exception("invalid hardware octave accepted"); }
        catch(IOException) {passed++;}
    }
}
using var unpluggedClient = new EditorClient(unplugged);
var waiting = unpluggedClient.RequestAsync(1, []);
unplugged.Unplug();
try { await waiting; throw new Exception("disconnect missing"); } catch (IOException) { passed++; }
using (var simulated = new SimulatedFm1())
using (var simulatedClient = new EditorClient(simulated))
{
    simulated.FragmentSize = 1;
    var simulatedInfo = await simulatedClient.GetInfoAsync();
    Check(simulatedInfo.Firmware == "FELUCCA " + simulated.Profile.Firmware &&
        simulatedInfo.ParameterCount == simulated.Profile.ParameterCount, "simulator firmware INFO");
    foreach (var parameter in simulated.Profile.Parameters)
    {
        var decoded = await simulatedClient.GetDescriptorAsync(parameter.Scope, parameter.Id);
        Check(decoded.Label == parameter.Label && decoded.Minimum == parameter.Minimum &&
            decoded.Maximum == parameter.Maximum && decoded.Choices.SequenceEqual(parameter.Choices), "firmware DESC fidelity");
    }
    Check(await simulatedClient.TrackParameterAsync(0, 0) == 104, "simulator level default");
    Check(await simulatedClient.TrackParameterAsync(1, 0, 8000) == 127, "simulator firmware clamping");
    Check(await simulatedClient.TrackParameterAsync(0, 0) == 104, "track isolation");
    Check(await simulatedClient.TrackParameterAsync(1, 0) == 127, "simulator state persists");
    await simulatedClient.RequestAsync(27, [1]);
    var selectedLevel = await simulatedClient.RequestAsync(2, [0, 0]);
    Check(EditorCodec.DecodeValue(selectedLevel.Arguments.AsSpan(2)) == 127, "selected track GET");
    simulated.ReplyDelay = TimeSpan.FromMilliseconds(5);
    Check((await simulatedClient.RequestAsync(25, [])).Arguments.SequenceEqual(new byte[] { 0 }), "delayed simulated reply");
    simulated.DropNextReply = true;
    try { await simulatedClient.RequestAsync(25, [], timeout: TimeSpan.FromMilliseconds(20)); throw new Exception("simulated drop ignored"); }
    catch (TimeoutException) { passed++; }
}
using (var simulated = new SimulatedFm1())
using (var simulatedClient = new EditorClient(simulated))
{
    simulated.ReplyDelay = TimeSpan.FromSeconds(1);
    var pending = simulatedClient.GetInfoAsync();
    simulated.Dispose();
    try { await pending; throw new Exception("simulated unplug ignored"); } catch (IOException) { passed++; }
}
var sampleDirectory = Path.Combine(Path.GetTempPath(), "sloop-sampling-" + Guid.NewGuid().ToString("N"));
Directory.CreateDirectory(sampleDirectory);
try
{
    var wavePath = Path.Combine(sampleDirectory, "original.wav");
    using (var writer = new BinaryWriter(File.Create(wavePath)))
    {
        writer.Write(Encoding.ASCII.GetBytes("RIFF")); writer.Write(58u);
        writer.Write(Encoding.ASCII.GetBytes("WAVEJUNK")); writer.Write(1u); writer.Write((byte)7); writer.Write((byte)0);
        writer.Write(Encoding.ASCII.GetBytes("fmt ")); writer.Write(16u); writer.Write((ushort)1); writer.Write((ushort)2);
        writer.Write(22050); writer.Write(88200); writer.Write((ushort)4); writer.Write((ushort)16);
        writer.Write(Encoding.ASCII.GetBytes("data")); writer.Write(12u);
        foreach (var value in new short[] { short.MinValue, 0, 100, -200, short.MaxValue, 10 }) writer.Write(value);
    }
    var original = File.ReadAllBytes(wavePath);
    var wave = PcmWave.Open(wavePath);
    Check(wave.Frames == 3 && wave.Channels == 2 && wave.SampleRate == 22050, "WAV unknown odd chunk and stereo frames");
    Check(wave.Peaks(3).SequenceEqual(new float[] { 1, 200 / 32768f, 32767 / 32768f }), "WAV stereo extrema including -32768");
    var slicePath = Path.Combine(sampleDirectory, "slice.wav");
    wave.Export(slicePath, 1, 3);
    var slice = PcmWave.Open(slicePath);
    Check(slice.Frames == 2 && File.ReadAllBytes(slicePath).AsSpan(44).SequenceEqual(original.AsSpan(54 + 4)), "slice frame-exact PCM extraction");
    Check(File.ReadAllBytes(wavePath).SequenceEqual(original), "slice export preserves original");
    var document = new SampleDocument(wave);
    document.EqualParts(3);
    Check(document.Slices().SequenceEqual(new (long, long)[] { (0, 1), (1, 2), (2, 3) }), "equal chops cover every frame once");
    document.Trim(1, 3);
    Check(document.Markers.SequenceEqual(new long[] { 2 }), "trim removes outside markers");
    Check(document.Undo() && document.Start == 0 && document.Markers.Count == 2, "undo restores removed markers and trim");
    Check(!document.Split(0) && !document.Split(3) && !document.Split(1), "empty and duplicate slices rejected");
    var editPath = Path.Combine(sampleDirectory, "edits");
    document.Save(editPath);
    var restored = SampleDocument.Restore(wave, editPath);
    Check(restored.Slices().SequenceEqual(document.Slices()) && !restored.CanUndo, "durable sample edits restore");
    File.WriteAllBytes(slicePath, original[..^1]);
    Reject(() => PcmWave.Open(slicePath), "truncated WAV rejected");
    var invalid = original.ToArray(); invalid[30] = 3; // fmt format code after padded JUNK
    File.WriteAllBytes(slicePath, invalid);
    Reject(() => PcmWave.Open(slicePath), "unsupported WAV format rejected");

    var take = CaptureJournal.Create(sampleDirectory, 48000, 1, "Microphone · test");
    var journalPath = take.JournalPath;
    take.Append(new byte[] { 1, 0, 255, 127, 0, 128 });
    try { take.Append(new byte[] { 1 }); throw new Exception("partial frame accepted"); }
    catch (ArgumentException) { passed++; }
    take.Dispose(); // Simulate process exit before WAV finalization/adoption.
    using var pendingTake = CaptureJournal.OpenPending(journalPath);
    var recoveredPath = pendingTake.FinalizeWave();
    var recoveredTake = PcmWave.Open(recoveredPath);
    Check(recoveredTake.SampleRate == 48000 && recoveredTake.Channels == 1 && recoveredTake.Frames == 3,
        "capture journal restores original stream format and complete frames");
    Check(File.ReadAllBytes(recoveredPath).AsSpan(44).SequenceEqual(new byte[] { 1, 0, 255, 127, 0, 128 }), "capture recovery preserves PCM exactly");
    Check(File.Exists(journalPath) && File.Exists(Path.ChangeExtension(journalPath, ".pcm")), "capture recovery retains journal until workspace adoption");
    Check(pendingTake.FinalizeWave() == recoveredPath, "capture finalization is restart-safe before adoption");
    pendingTake.Commit("Recovered interruption");
    Check(!File.Exists(journalPath) && File.Exists(recoveredPath + ".capture") && File.Exists(recoveredPath), "capture commit preserves WAV and provenance");
    using var stereoTake = CaptureJournal.Create(sampleDirectory, 44100, 2, "USB test");
    stereoTake.Append(new byte[] { 1, 0, 2, 0 }); stereoTake.Dispose();
    using (var partial = new FileStream(Path.ChangeExtension(stereoTake.JournalPath, ".pcm"), FileMode.Append)) partial.WriteByte(99);
    using var pendingStereo = CaptureJournal.OpenPending(stereoTake.JournalPath);
    Check(PcmWave.Open(pendingStereo.FinalizeWave()).Frames == 1, "recovery discards incomplete trailing stereo frame");
}
finally { Directory.Delete(sampleDirectory, recursive: true); }
Console.WriteLine($"PASS: {passed} protocol/simulator/sampling checks.");

sealed class FakeTransport : IMidiTransport
{
    public event Action<ReadOnlyMemory<byte>>? BytesReceived;
    public event Action? Disconnected;
    public List<byte[]> Sent { get; } = [];
    public ValueTask SendAsync(ReadOnlyMemory<byte> bytes, CancellationToken token)
    { token.ThrowIfCancellationRequested(); lock (Sent) Sent.Add(bytes.ToArray()); return ValueTask.CompletedTask; }
    public void Receive(byte[] bytes) => BytesReceived?.Invoke(bytes);
    public void Unplug() => Disconnected?.Invoke();
    public async Task WaitForSends(int count)
    {
        using var timeout = new CancellationTokenSource(TimeSpan.FromSeconds(2));
        while (true)
        {
            lock (Sent) if (Sent.Count >= count) return;
            await Task.Delay(1, timeout.Token);
        }
    }
}
