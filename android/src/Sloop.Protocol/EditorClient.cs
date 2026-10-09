namespace Sloop.Protocol;

public interface IMidiTransport
{
    event Action<ReadOnlyMemory<byte>>? BytesReceived;
    event Action? Disconnected;
    ValueTask SendAsync(ReadOnlyMemory<byte> bytes, CancellationToken cancellationToken);
}

/// <summary>One request in flight. Ambiguous failure faults this epoch; reconnect before reuse.</summary>
public sealed partial class EditorClient : IDisposable
{
    private readonly IMidiTransport transport;
    private readonly EditorStreamParser parser = new();
    private readonly SemaphoreSlim serial = new(1, 1);
    private readonly object sync = new();
    private TaskCompletionSource<EditorFrame>? pending;
    private byte expectedCommand;
    private Func<EditorFrame, bool>? matches;
    private bool faulted;
    private bool disposed;
    public event Action<EditorFrame>? NotificationReceived;

    public EditorClient(IMidiTransport transport)
    {
        this.transport = transport;
        parser.FrameReceived += OnFrame;
        transport.BytesReceived += OnBytes;
        transport.Disconnected += OnDisconnected;
    }

    public async Task<EditorFrame> RequestAsync(byte command, byte[] arguments,
        Func<EditorFrame, bool>? replyMatches = null, TimeSpan? timeout = null,
        CancellationToken cancellationToken = default)
    {
        if (command is 23 or 24 or 26 or 32) throw new ArgumentException("Command is a push notification.", nameof(command));
        var wire = EditorCodec.Encode(command, arguments);
        await serial.WaitAsync(cancellationToken).ConfigureAwait(false);
        TaskCompletionSource<EditorFrame>? completion = null;
        try
        {
            lock (sync)
            {
                ObjectDisposedException.ThrowIf(disposed, this);
                if (faulted) throw new IOException("MIDI session is uncertain. Disconnect and reconnect.");
                expectedCommand = command; matches = replyMatches;
                pending = completion = new(TaskCreationOptions.RunContinuationsAsynchronously);
            }
            await transport.SendAsync(wire, cancellationToken).ConfigureAwait(false);
            return await completion.Task.WaitAsync(timeout ?? TimeSpan.FromSeconds(3), cancellationToken).ConfigureAwait(false);
        }
        catch
        {
            // Cancellation after send can leave an unidentifiable late reply, just like timeout.
            if (completion is not null) lock (sync) faulted = true;
            throw;
        }
        finally
        {
            lock (sync) { pending = null; matches = null; }
            serial.Release();
        }
    }

    public async Task<DeviceInfo> GetInfoAsync(CancellationToken token = default) =>
        DeviceInfo.Parse(await RequestAsync(1, [], cancellationToken: token).ConfigureAwait(false));

    public async Task<int> GetHardwareOctaveAsync(CancellationToken token = default)
    {
        var frame = await RequestAsync(43, [], cancellationToken: token).ConfigureAwait(false);
        if (frame.Arguments.Length != 2) throw new IOException("Invalid performance-state reply.");
        int offset = EditorCodec.DecodeValue(frame.Arguments);
        if (offset is < -3 or > 3) throw new IOException("Invalid hardware octave offset.");
        return offset;
    }

    public async Task<ParameterDescriptor> GetDescriptorAsync(byte scope, byte id, CancellationToken token = default) =>
        ParameterDescriptor.Parse(await RequestAsync(5, [scope, id],
            f => f.Arguments.Length >= 2 && f.Arguments[0] == scope && f.Arguments[1] == id,
            cancellationToken: token).ConfigureAwait(false));

    public async Task<int> TrackParameterAsync(byte track, byte id, int? value = null, CancellationToken token = default)
    {
        byte[] args = value is null ? [track, id] : [track, id, .. EditorCodec.EncodeValue(value.Value)];
        var frame = await RequestAsync(31, args,
            f => f.Arguments.Length == 4 && f.Arguments[0] == track && f.Arguments[1] == id,
            cancellationToken: token).ConfigureAwait(false);
        return EditorCodec.DecodeValue(frame.Arguments.AsSpan(2));
    }

    private void OnBytes(ReadOnlyMemory<byte> bytes) { lock (sync) if (!disposed) parser.Feed(bytes.Span); }
    private void OnFrame(EditorFrame frame)
    {
        if (frame.Command is 23 or 24 or 26 or 32) { NotificationReceived?.Invoke(frame); return; }
        if (!faulted && pending is not null && frame.Command == expectedCommand && (matches?.Invoke(frame) ?? true))
            pending.TrySetResult(frame);
    }
    private void OnDisconnected()
    {
        lock (sync) { faulted = true; pending?.TrySetException(new IOException("MIDI device disconnected.")); }
    }
    public void Dispose()
    {
        lock (sync)
        {
            if (disposed) return;
            disposed = true;
            pending?.TrySetException(new ObjectDisposedException(nameof(EditorClient)));
            transport.BytesReceived -= OnBytes;
            transport.Disconnected -= OnDisconnected;
            parser.FrameReceived -= OnFrame;
        }
        // Semaphore is kept alive until any queued callers have observed disposed state.
    }
}
