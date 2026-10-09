using Android.Content;
using Android.Media;
using Android.OS;
using Sloop.Core.Sampling;

namespace Sloop.Android.Services;

/// <summary>Process-owned sample workspace; durable original and edits survive activity recreation.</summary>
public sealed partial class SampleWorkspace
{
    private static SampleWorkspace? instance;
    public static SampleWorkspace Get(Context context) {SessionWorkspace.RecoverStartup(context.FilesDir!.AbsolutePath);return instance ??= new(context.ApplicationContext!);}
    private readonly Context context;
    private string directory;
    private readonly Handler main = new(Looper.MainLooper!);
    private MediaPlayer? player;
    private int previewEpoch;
    private readonly AudioManager? audioManager;
    private readonly OutputEvents outputEvents;
    private readonly RoutingEvents routingEvents;
    private int? outputId;
    private bool routeConfirmed;
    public string OutputName { get; private set; } = "System default";
    public AudioDeviceInfo[] Outputs() => audioManager?.GetDevices(GetDevicesTargets.Outputs) ?? [];
    public void SelectOutput(AudioDeviceInfo? device)
    {
        if (Busy) return;
        if (device is not null && !OperatingSystem.IsAndroidVersionAtLeast(28))
        {
            Status = "Explicit preview output selection requires Android 9 or later."; Notify(); return;
        }
        StopPreview(); outputId = device?.Id;
        OutputName = device is null ? "System default" : $"{device.ProductName} · {device.Type}";
        Status = $"Preview output selected: {OutputName}."; Notify();
    }
    public SampleDocument? Document { get; private set; }
    public float[] Peaks { get; private set; } = [];
    public bool Busy { get; private set; }
    public string Status { get; private set; } = "Import a mono or stereo PCM16 WAV to begin.";
    public long Cursor { get; set; }
    public (PcmWave Source, long Start, long End)? PendingExport { get; set; }
    public event Action? Changed;

    private SampleWorkspace(Context context)
    {
        this.context = context;
        audioManager = context.GetSystemService(Context.AudioService) as AudioManager;
        outputEvents = new(this); routingEvents = new(this);
        audioManager?.RegisterAudioDeviceCallback(outputEvents, main);
        directory = System.IO.Path.Combine(new Sloop.Workstation.WorkspaceGenerationStore(context.FilesDir!.AbsolutePath).ActiveRoot, "samples");
        Directory.CreateDirectory(directory);
        _ = RestoreAsync();
    }
    private string Pointer => System.IO.Path.Combine(directory, "current.txt");
    private string Edits => Document!.Source.Path + ".edits";
    private void Notify() => main.Post(() => Changed?.Invoke());
    private async Task RestoreAsync()
    {
        Busy = true;
        try
        {
            if (await RecoverCaptureAsync()) return;
            if (File.Exists(Pointer))
            {
                var name = File.ReadAllText(Pointer);
                if (System.IO.Path.GetFileName(name) != name) throw new IOException("Invalid saved asset path.");
                var source = PcmWave.Open(System.IO.Path.Combine(directory, name));
                Document = File.Exists(source.Path + ".edits") ? SampleDocument.Restore(source, source.Path + ".edits") : new(source);
                Peaks = await Task.Run(() => source.Peaks());
                Status = "Saved sample restored.";
            }
        }
        catch (Exception error) { Document = null; Status = "Could not restore sample: " + error.Message; }
        finally { Busy = false; Notify(); }
    }
    public async Task ImportAsync(global::Android.Net.Uri uri)
    {
        if (Busy) return;
        Busy = true; StopPreview(); Status = "Copying and analysing WAV…"; Notify();
        var destination = System.IO.Path.Combine(directory, Guid.NewGuid().ToString("N") + ".wav");
        try
        {
            var result = await Task.Run(() =>
            {
                using var input = context.ContentResolver!.OpenInputStream(uri) ?? throw new IOException("Cannot open selected file.");
                using (var output = File.Create(destination))
                {
                    var buffer = new byte[65536]; long total = 0; int count;
                    while ((count = input.Read(buffer, 0, buffer.Length)) > 0)
                    {
                        total += count;
                        if (total > 256L * 1024 * 1024) throw new IOException("This version accepts WAV files up to 256 MB.");
                        output.Write(buffer, 0, count);
                    }
                }
                var source = PcmWave.Open(destination);
                return (Source: source, Peaks: source.Peaks());
            });
            var document = new SampleDocument(result.Source);
            document.Save(destination + ".edits");
            File.WriteAllText(Pointer + ".tmp", System.IO.Path.GetFileName(destination));
            File.Move(Pointer + ".tmp", Pointer, true);
            Document = document; Peaks = result.Peaks; Cursor = 0;
            Status = "Imported. Tap the waveform to position a chop marker.";
        }
        catch (Exception error)
        {
            File.Delete(destination); File.Delete(destination + ".edits");
            Status = "Import failed: " + error.Message;
        }
        finally { Busy = false; Notify(); }
    }
    public void Edit(Action<SampleDocument> edit)
    {
        if (Busy || Document is null) return;
        StopPreview();
        try { edit(Document); Document.Save(Edits); Status = "Edits saved; original WAV preserved."; }
        catch (Exception error) { Status = "Edit failed: " + error.Message; }
        Notify();
    }
    public async Task PreviewAsync(long start, long end, PcmWave? alternate=null)
    {
        if (Busy || Document is null) return;
        Busy = true; StopPreview(); Status = "Preparing local preview…"; Notify();
        var epoch = previewEpoch;
        var source = alternate ?? Document.Source;
        var path = System.IO.Path.Combine(context.CacheDir!.AbsolutePath, "slice-preview.wav");
        try
        {
            await Task.Run(() => source.Export(path, start, end));
            if (epoch != previewEpoch) { Status = "Preview cancelled."; return; }
            var requested = outputId;
            var device = requested is null ? null : Outputs().FirstOrDefault(d => d.Id == requested);
            if (requested is not null && device is null) throw new IOException("Selected output is disconnected. Choose an available output.");
            player = new MediaPlayer();
            var audition = player;
            player.Completion += (sender, _) =>
            {
                if (!ReferenceEquals(player, sender)) return;
                player.Release(); player.Dispose(); player = null;
                routeConfirmed = false;
                Status = "Preview complete."; Notify();
            };
            player.SetAudioAttributes(new AudioAttributes.Builder()!.SetUsage(AudioUsageKind.Media)!
                .SetContentType(AudioContentType.Music)!.Build()!);
            player.SetDataSource(path);
            if (device is not null && (!OperatingSystem.IsAndroidVersionAtLeast(28) || !player.SetPreferredDevice(device)))
                throw new IOException("Android rejected the selected output.");
            if (OperatingSystem.IsAndroidVersionAtLeast(28)) player.AddOnRoutingChangedListener(routingEvents, main);
            if (requested is not null) player.SetVolume(0, 0);
            player.Prepare(); player.Start();
            previewSource = source; previewStart = start; previewEnd = end;
            if (!OperatingSystem.IsAndroidVersionAtLeast(28))
            {
                Status = "Playing through system output. Explicit route selection requires Android 9 or later."; return;
            }
            // Preferred-device acceptance does not prove that the stream uses that route.
            for (int attempt = 0; attempt < 25; attempt++)
            {
                await Task.Delay(20);
                if (epoch != previewEpoch || !ReferenceEquals(player, audition)) return;
                var actual = audition.RoutedDevice;
                if (actual is null || requested is not null && actual.Id != requested) continue;
                routeConfirmed = true;
                if (requested is not null) { audition.SeekTo(0); audition.SetVolume(1, 1); }
                Status = $"Playing WAV through {actual.ProductName} · {actual.Type}.";
                return;
            }
            if (requested is not null) throw new IOException("Could not confirm selected output. Playback stopped without falling back.");
            Status = "Playing through system output; route identity is unavailable.";
        }
        catch (Exception error) { StopPreview(); Status = "Preview failed: " + error.Message; }
        finally { Busy = false; Notify(); }
    }
    public void StopPreview()
    {
        previewSource = null;
        previewEpoch++;
        routeConfirmed = false;
        if (player is not null)
        {
            player.Release(); player.Dispose(); player = null;
            Status = "Preview stopped."; Notify();
        }
    }

    private sealed class OutputEvents(SampleWorkspace owner) : AudioDeviceCallback
    {
        public override void OnAudioDevicesAdded(AudioDeviceInfo[]? addedDevices) => owner.Notify();
        public override void OnAudioDevicesRemoved(AudioDeviceInfo[]? removedDevices)
        {
            if (owner.outputId is { } selected && removedDevices?.Any(d => d.Id == selected) == true)
            {
                owner.StopPreview();
                owner.Status = "Selected output disconnected. Playback stopped; choose an output to continue.";
            }
            owner.Notify();
        }
    }
    private sealed class RoutingEvents(SampleWorkspace owner) : Java.Lang.Object, IAudioRoutingOnRoutingChangedListener
    {
        public void OnRoutingChanged(IAudioRouting? router)
        {
            if (!OperatingSystem.IsAndroidVersionAtLeast(28)) return;
            if (!owner.routeConfirmed || owner.outputId is not { } selected || owner.player is null) return;
            if (owner.player.RoutedDevice?.Id != selected)
            {
                owner.StopPreview(); owner.Status = "Preview output changed. Playback stopped; select the output again.";
                owner.Notify();
            }
        }
    }

    public async Task ExportAsync(global::Android.Net.Uri uri)
    {
        var pending = PendingExport; PendingExport = null;
        if (Busy || pending is null) return;
        Busy = true; Status = "Exporting WAV…"; Notify();
        var path = System.IO.Path.Combine(context.CacheDir!.AbsolutePath, "slice-export.wav");
        try
        {
            await Task.Run(() =>
            {
                pending.Value.Source.Export(path, pending.Value.Start, pending.Value.End);
                using var input = File.OpenRead(path);
                using var output = context.ContentResolver!.OpenOutputStream(uri, "wt") ?? throw new IOException("Cannot write selected destination.");
                input.CopyTo(output);
            });
            Status = "Exported PCM16 WAV with original sample rate and channels.";
        }
        catch (Exception error) { Status = "Export failed: " + error.Message; }
        finally { File.Delete(path); Busy = false; Notify(); }
    }
}
