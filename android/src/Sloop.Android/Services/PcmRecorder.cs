using Android.Media;
using System.Buffers.Binary;
using System.Threading.Channels;
using Sloop.Core.Sampling;

namespace Sloop.Android.Services;

public sealed record CaptureProgress(double Seconds, int Peak, string Route, int Rate, int Channels);
public sealed record CaptureResult(CaptureJournal Journal, string Outcome);

/// <summary>Bounded capture-to-writer queue; capture never waits for storage.</summary>
internal sealed class PcmRecorder : IDisposable
{
    private readonly object sync = new();
    private readonly CancellationTokenSource stop = new();
    private AudioRecord? active;
    private string outcome = "Stopped by user";
    public void Stop(string reason)
    {
        lock (sync)
        {
            if (stop.IsCancellationRequested) return;
            outcome = reason; stop.Cancel();
            try { active?.Stop(); } catch (Java.Lang.IllegalStateException) { }
        }
    }
    public Task<CaptureResult> RecordAsync(string directory, AudioDeviceInfo device, bool unprocessed,
        Action<CaptureProgress> report) => Task.Run(async () =>
    {
        stop.Token.ThrowIfCancellationRequested();
        bool usb = device.Type is AudioDeviceType.UsbDevice or AudioDeviceType.UsbHeadset;
        int rate = usb ? 44100 : 48000, channels = usb ? 2 : 1;
        var mask = channels == 1 ? ChannelIn.Mono : ChannelIn.Stereo;
        var source = unprocessed && !usb ? AudioSource.Unprocessed : AudioSource.VoiceRecognition;
        int minimum = AudioRecord.GetMinBufferSize(rate, mask, global::Android.Media.Encoding.Pcm16bit);
        if (minimum <= 0) throw new IOException("Requested capture format is unavailable.");
        using var record = new AudioRecord(source, rate, mask, global::Android.Media.Encoding.Pcm16bit, Math.Max(minimum * 2, 16384));
        using var nativeResources = new RecordResources(record);
        if (record.State != State.Initialized || !record.SetPreferredDevice(device))
            throw new IOException("Android could not open the selected recording input.");
        rate = record.SampleRate; channels = record.ChannelCount;
        var description = $"{device.ProductName} · {device.Type} · {(source == AudioSource.Unprocessed ? "unprocessed" : "voice-recognition source; processing may apply")}";
        using var journal = CaptureJournal.Create(directory, rate, channels, description);
        var queue = Channel.CreateBounded<byte[]>(new BoundedChannelOptions(16)
        { SingleReader = true, SingleWriter = true, FullMode = BoundedChannelFullMode.Wait });
        Exception? failure = null;
        var writer = Task.Run(async () =>
        {
            try { await foreach (var buffer in queue.Reader.ReadAllAsync()) journal.Append(buffer); }
            catch (Exception error) { Interlocked.CompareExchange(ref failure, error, null); Stop("Storage writer failed"); }
        });
        long frames = 0; long maximum = rate * 120L;
        try
        {
            lock (sync) { stop.Token.ThrowIfCancellationRequested(); active = record; record.StartRecording(); }
            if (record.RecordingState != RecordState.Recording) throw new IOException("Recording did not start.");
            while (!stop.IsCancellationRequested && frames < maximum)
            {
                var buffer = new byte[(int)Math.Min(4096L, (maximum - frames) * channels * 2)];
                int read = record.Read(buffer, 0, buffer.Length);
                if (stop.IsCancellationRequested) break;
                if (read <= 0 || read % (channels * 2) != 0) throw new IOException($"Audio capture failed ({read}).");
                if (record.RoutedDevice?.Id != device.Id) throw new IOException("Recording input changed or could not be verified. Take stopped.");
                if (OperatingSystem.IsAndroidVersionAtLeast(29) && record.ActiveRecordingConfiguration?.IsClientSilenced == true)
                    throw new IOException("Android silenced recording. Check microphone privacy and competing apps.");
                if (read != buffer.Length) Array.Resize(ref buffer, read);
                if (!queue.Writer.TryWrite(buffer)) throw new IOException("Storage could not keep up. Take stopped at the last contiguous audio block.");
                frames += read / (channels * 2);
                int peak = 0;
                for (int i = 0; i < read; i += 2) peak = Math.Max(peak, Math.Abs((int)BinaryPrimitives.ReadInt16LittleEndian(buffer.AsSpan(i, 2))));
                report(new(frames / (double)rate, peak, description, rate, channels));
            }
            if (frames >= maximum) outcome = "Two-minute take limit reached";
        }
        catch (OperationCanceledException) when (stop.IsCancellationRequested) { }
        catch (Exception error) { Interlocked.CompareExchange(ref failure, error, null); }
        finally
        {
            lock (sync)
            {
                active = null;
                try { record.Stop(); } catch (Java.Lang.IllegalStateException) { }
            }
            queue.Writer.TryComplete();
        }
        await writer;
        journal.FinalizeWave();
        var errorOutcome = failure is null ? outcome : "Interrupted: " + failure.Message;
        return new CaptureResult(journal, errorOutcome);
    });
    private sealed class RecordResources(AudioRecord record) : IDisposable
    {
        public void Dispose() => record.Release();
    }
    public void Dispose() { Stop("Recording interrupted"); stop.Dispose(); }
}
