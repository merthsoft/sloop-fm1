using Android.Media.Midi;
using Android.OS;
using Sloop.Protocol;

namespace Sloop.Android.Services;

/// <summary>Android ports expose raw MIDI bytes, not USB event packets.</summary>
internal sealed class AndroidMidiTransport : IMidiTransport, IDisposable
{
    private readonly object sync = new();
    private readonly MidiDevice device;
    private readonly MidiInputPort input;
    private readonly MidiOutputPort? output;
    private readonly Receiver? receiver;
    private bool closed;
    public event Action<ReadOnlyMemory<byte>>? BytesReceived;
    public event Action? Disconnected;

    private AndroidMidiTransport(MidiDevice device, bool receive)
    {
        this.device = device;
        input = device.OpenInputPort(0) ?? throw new IOException("Device MIDI input port is unavailable.");
        if (!receive) return;
        try
        {
            output = device.OpenOutputPort(0) ?? throw new IOException("FM1 MIDI output port is unavailable.");
            receiver = new Receiver(this);
            output.Connect(receiver);
        }
        catch { output?.Close(); receiver?.Dispose(); input.Close(); throw; }
    }

    public static async Task<AndroidMidiTransport> OpenAsync(MidiManager manager, MidiDeviceInfo info,
        Handler handler, CancellationToken token, bool receive = true)
    {
        var listener = new OpenListener();
        manager.OpenDevice(info, listener, handler);
        MidiDevice? device = null;
        try
        {
            device = await listener.Completion.Task.WaitAsync(TimeSpan.FromSeconds(5), token);
            return new AndroidMidiTransport(device, receive);
        }
        catch { listener.Abandon(); throw; }
        // Listener lifetime extends to Android's callback; do not dispose it on timeout.
    }

    public ValueTask SendAsync(ReadOnlyMemory<byte> bytes, CancellationToken token)
    {
        token.ThrowIfCancellationRequested();
        var data = bytes.ToArray();
        lock (sync)
        {
            ObjectDisposedException.ThrowIf(closed, this);
            input.Send(data, 0, data.Length);
        }
        return ValueTask.CompletedTask;
    }

    public void Dispose()
    {
        lock (sync)
        {
            if (closed) return;
            closed = true;
        }
        // Notify outside the port lock; the protocol receive path has its own lock.
        Disconnected?.Invoke();
        try { if(receiver is not null) output?.Disconnect(receiver); } catch (Java.IO.IOException) { }
        try { output?.Close(); } catch (Java.IO.IOException) { }
        try { input.Close(); } catch (Java.IO.IOException) { }
        try { device.Close(); } catch (Java.IO.IOException) { }
        receiver?.Dispose();
    }

    private sealed class Receiver(AndroidMidiTransport owner) : MidiReceiver
    {
        public override void OnSend(byte[]? data, int offset, int count, long timestamp)
        {
            if (data is null || count == 0) return;
            // Copy before returning; Android owns and can reuse the incoming buffer.
            owner.BytesReceived?.Invoke(data.AsMemory(offset, count).ToArray());
        }
    }

    private sealed class OpenListener : Java.Lang.Object, MidiManager.IOnDeviceOpenedListener
    {
        private readonly object sync = new();
        private bool abandoned;
        public TaskCompletionSource<MidiDevice> Completion { get; } = new(TaskCreationOptions.RunContinuationsAsynchronously);
        public void OnDeviceOpened(MidiDevice? device)
        {
            lock (sync)
            {
                if (abandoned) { device?.Close(); return; }
                if (device is null) Completion.TrySetException(new IOException("Android could not open the MIDI device."));
                else if (!Completion.TrySetResult(device)) device.Close();
            }
        }
        public void Abandon()
        {
            lock (sync)
            {
                abandoned = true;
                if (Completion.Task.IsCompletedSuccessfully) Completion.Task.Result.Close();
            }
        }
    }
}
