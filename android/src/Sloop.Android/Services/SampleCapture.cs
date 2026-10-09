using Android.Media;
using Sloop.Core.Sampling;

namespace Sloop.Android.Services;

public sealed partial class SampleWorkspace
{
    private PcmRecorder? capture;
    private bool captureFinalizing;
    public bool RecordingActive => capture is not null && !captureFinalizing;
    public int CapturePeak { get; private set; }
    public int? PendingCaptureInput { get; set; }
    public AudioDeviceInfo[] Inputs() => (audioManager?.GetDevices(GetDevicesTargets.Inputs) ?? [])
        .Where(d => d.Type is AudioDeviceType.BuiltinMic or AudioDeviceType.UsbDevice or AudioDeviceType.UsbHeadset).ToArray();

    public async Task RecordAsync(int deviceId)
    {
        if (Busy) return;
        var device = Inputs().FirstOrDefault(d => d.Id == deviceId);
        if (device is null) { Status = "Recording input disconnected. Choose an available input."; Notify(); return; }
        Busy = true; StopPreview(); CapturePeak = 0; captureFinalizing = false;
        var recorder = capture = new PcmRecorder();
        Status = $"Opening {device.ProductName} · {device.Type}…"; Notify();
        try
        {
            bool unprocessed = string.Equals(audioManager?.GetProperty(AudioManager.PropertySupportAudioSourceUnprocessed!),
                "true", StringComparison.OrdinalIgnoreCase);
            var result = await recorder.RecordAsync(directory, device, unprocessed, progress => main.Post(() =>
            {
                if (!ReferenceEquals(capture, recorder) || !RecordingActive) return;
                CapturePeak = progress.Peak;
                Status = $"Recording · {progress.Seconds:0.0} s / 120 s\n{progress.Rate} Hz · {progress.Channels} channel(s)\n{progress.Route}";
                Notify();
            }));
            captureFinalizing = true; Status = "Finalizing take and waveform…"; Notify();
            var source = PcmWave.Open(result.Journal.WavePath);
            var peaks = await Task.Run(() => source.Peaks());
            AdoptTake(source, peaks);
            result.Journal.Commit(result.Outcome);
            Status = $"Take ready · {source.Frames / (double)source.SampleRate:0.00} s. {result.Outcome}.\nOriginal retained; trim or chop below.";
        }
        catch (Exception error) { Status = "Recording failed: " + error.Message + " Existing sample preserved; any written PCM remains available for recovery."; }
        finally { capture = null; captureFinalizing = false; recorder.Dispose(); CapturePeak = 0; Busy = false; Notify(); }
    }
    public void StopRecording(string reason = "Stopped by user")
    {
        if (capture is null || captureFinalizing) return;
        captureFinalizing = true; capture.Stop(reason);
        Status = "Stopping and finalizing recording…"; Notify();
    }
    public void CapturePermissionDenied()
    {
        PendingCaptureInput = null;
        Status = "Recording permission was not granted. Import and editing remain available."; Notify();
    }
    private void AdoptTake(PcmWave source, float[] peaks)
    {
        var document = File.Exists(source.Path + ".edits") ? SampleDocument.Restore(source, source.Path + ".edits") : new(source);
        document.Save(source.Path + ".edits");
        File.WriteAllText(Pointer + ".tmp", System.IO.Path.GetFileName(source.Path));
        File.Move(Pointer + ".tmp", Pointer, true);
        Document = document; Peaks = peaks; Cursor = document.Start;
    }
    private async Task<bool> RecoverCaptureAsync()
    {
        var pending = await Task.Run(() =>
        {
            var candidates = new List<CaptureJournal>();
            foreach (var path in Directory.EnumerateFiles(directory, "*.take").OrderBy(File.GetLastWriteTimeUtc))
            {
                try
                {
                    var journal = CaptureJournal.OpenPending(path);
                    journal.FinalizeWave(); candidates.Add(journal);
                }
                catch (Exception error) when (error is IOException or FormatException or OverflowException)
                { /* Keep failed/empty raw takes intact; they must not block restoring a valid sample. */ }
            }
            return candidates;
        });
        if (pending.Count == 0) return false;
        var recovered = PcmWave.Open(pending[^1].WavePath);
        var peaks = await Task.Run(() => recovered.Peaks());
        AdoptTake(recovered, peaks);
        foreach (var take in pending) take.Commit("Recovered after recording interruption; complete written frames retained");
        Status = "Recovered interrupted recording. Inspect the take before using it.";
        return true;
    }
}
