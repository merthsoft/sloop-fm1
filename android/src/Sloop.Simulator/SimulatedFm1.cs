using System.Text;
using Sloop.Protocol;

namespace Sloop.Simulator;

/// <summary>Protocol-level FM1 model; does not execute the firmware DSP or emulate USB.</summary>
public sealed class SimulatedFm1 : IMidiTransport, IDisposable
{
    private readonly object sync = new();
    private readonly EditorStreamParser parser = new();
    private readonly CancellationTokenSource lifetime = new();
    private readonly Dictionary<(byte Scope, byte Id), SimulatorParameter> descriptors;
    private readonly Dictionary<(byte Track, byte Id), int> tracks = [];
    private readonly Dictionary<byte, int> globals = [];
    private bool disposed;
    private byte selected;
    public FirmwareProfile Profile { get; }
    public TimeSpan ReplyDelay { get; set; } = TimeSpan.Zero;
    public int FragmentSize { get; set; } = 3;
    public bool DropNextReply { get; set; }
    public event Action<ReadOnlyMemory<byte>>? BytesReceived;
    public event Action? Disconnected;

    public SimulatedFm1(FirmwareProfile? profile = null)
    {
        Profile = profile ?? FirmwareProfile.LoadDefault();
        descriptors = Profile.Parameters.ToDictionary(p => (p.Scope, p.Id));
        foreach (var parameter in Profile.Parameters)
        {
            if (parameter.Scope == 1) globals[parameter.Id] = parameter.Default;
            else for (byte track = 0; track < Profile.TrackCount; track++)
                tracks[(track, parameter.Id)] = parameter.Default;
        }
        parser.FrameReceived += OnRequest;
    }

    public ValueTask SendAsync(ReadOnlyMemory<byte> bytes, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        lock (sync)
        {
            ObjectDisposedException.ThrowIf(disposed, this);
            parser.Feed(bytes.Span);
        }
        return ValueTask.CompletedTask;
    }

    private void OnRequest(EditorFrame frame)
    {
        var reply = Handle(frame);
        if (reply is null) return; // Unsupported operation: no fabricated success.
        if (DropNextReply) { DropNextReply = false; return; }
        var wire = EditorCodec.Encode(frame.Command, reply);
        var delay = ReplyDelay;
        var fragment = Math.Max(1, FragmentSize);
        if (delay <= TimeSpan.Zero) Deliver(wire, fragment);
        else _ = DeliverLaterAsync(wire, fragment, delay, lifetime.Token);
    }

    private async Task DeliverLaterAsync(byte[] bytes, int fragment, TimeSpan delay, CancellationToken token)
    {
        try
        {
            await Task.Delay(delay, token).ConfigureAwait(false);
            Deliver(bytes, fragment);
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested) { }
    }

    private void Deliver(byte[] bytes, int fragment)
    {
        lock (sync)
        {
            if (disposed) return;
            for (var offset = 0; offset < bytes.Length; offset += fragment)
                BytesReceived?.Invoke(bytes.AsMemory(offset, Math.Min(fragment, bytes.Length - offset)));
        }
    }

    private byte[]? Handle(EditorFrame frame)
    {
        var args = frame.Arguments;
        switch (frame.Command)
        {
            case 1 when args.Length == 0:
            {
                var result = new List<byte>();
                String(result, "FELUCCA " + Profile.Firmware);
                result.AddRange([(byte)Profile.Engines.Length, Profile.ParameterCount, Profile.GlobalCount,
                    Profile.StepCount, Profile.EngineParameterStart]);
                foreach (var engine in Profile.Engines) String(result, engine);
                result.AddRange([Profile.TrackCount, Profile.ProtocolVersion]);
                return result.ToArray();
            }
            case 5 when args.Length == 2 && descriptors.TryGetValue((args[0], args[1]), out var descriptor):
            {
                var result = new List<byte> { descriptor.Scope, descriptor.Id, descriptor.Format };
                result.AddRange(EditorCodec.EncodeValue(descriptor.Minimum));
                result.AddRange(EditorCodec.EncodeValue(descriptor.Maximum));
                result.AddRange(EditorCodec.EncodeValue(descriptor.Default));
                String(result, descriptor.Label); String(result, descriptor.Unit);
                if (descriptor.Format == 8) foreach (var choice in descriptor.Choices) String(result, choice);
                return result.ToArray();
            }
            case 2 when args.Length == 2:
            case 3 when args.Length == 4:
            {
                if (!descriptors.TryGetValue((args[0], args[1]), out var descriptor)) return null;
                // Action globals require firmware execution and are intentionally unsupported here.
                if (descriptor.Symbol is "G_LOAD" or "G_SAVE" or "G_ENGSEL" or "G_ENGGO" or
                    "G_CLRSEQ" or "G_INITSND" or "G_NEWPRJ") return null;
                var value = args[0] == 0 ? tracks[(selected, args[1])] : globals[args[1]];
                if (frame.Command == 3)
                {
                    value = Math.Clamp(EditorCodec.DecodeValue(args.AsSpan(2)), descriptor.Minimum, descriptor.Maximum);
                    if (args[0] == 0) tracks[(selected, args[1])] = value;
                    else globals[args[1]] = value;
                }
                return [args[0], args[1], .. EditorCodec.EncodeValue(value)];
            }
            case 25 when args.Length == 0: return [0];
            case 27 when args.Length == 0 || args.Length == 1 && args[0] < Profile.TrackCount:
            {
                if (args.Length == 1) selected = args[0];
                var levelId = Profile.Parameters.Single(p => p.Symbol == "P_LEVEL").Id;
                var muteId = Profile.Parameters.Single(p => p.Symbol == "P_MUTE").Id;
                var result = new List<byte> { selected, Profile.TrackCount };
                for (byte track = 0; track < Profile.TrackCount; track++)
                {
                    result.Add(track == 3 ? (byte)Profile.Engines.Length : (byte)0);
                    result.Add(0); // No factory preset emulation.
                    var level = track == 3 ? globals[Profile.Parameters.Single(p => p.Symbol == "G_DRLVL").Id] : tracks[(track, levelId)];
                    result.AddRange(EditorCodec.EncodeValue(level));
                    result.Add((byte)tracks[(track, muteId)]); result.Add(0); // Not recording.
                }
                result.Add(0); // No solos.
                return result.ToArray();
            }
            case 31 when args.Length is 2 or 4 && args[0] < Profile.TrackCount:
            {
                if (!descriptors.TryGetValue((0, args[1]), out var descriptor)) return null;
                var key = (args[0], args[1]);
                if (args.Length == 4)
                    tracks[key] = Math.Clamp(EditorCodec.DecodeValue(args.AsSpan(2)), descriptor.Minimum, descriptor.Maximum);
                return [args[0], args[1], .. EditorCodec.EncodeValue(tracks[key])];
            }
            default: return null;
        }
    }

    private static void String(List<byte> result, string value)
    {
        result.AddRange(Encoding.ASCII.GetBytes(value)); result.Add(0);
    }

    public void Dispose()
    {
        lock (sync)
        {
            if (disposed) return;
            disposed = true;
            lifetime.Cancel();
            parser.FrameReceived -= OnRequest;
        }
        Disconnected?.Invoke();
        // Keep the cancellation source alive for outstanding delayed callbacks to observe it.
    }
}
