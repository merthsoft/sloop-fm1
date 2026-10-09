using Sloop.Core.Sampling;
using Sloop.Workstation;
using Sloop.SampleEncoding;
using System.Globalization;

namespace Sloop.Android.Services;

public sealed partial class SampleWorkspace
{
    public sealed record RetainedAsset(string Name, double Seconds, int Chops, bool Current);
    public async Task<IReadOnlyList<RetainedAsset>> RetainedAssetsAsync()
    {
        var ownedDirectory = directory;
        var currentPath = Document?.Source.Path;
        return await Task.Run(() => {
            var assets = new List<RetainedAsset>();
            foreach (var path in Directory.EnumerateFiles(ownedDirectory, "*.wav").OrderByDescending(File.GetLastWriteTimeUtc))
            {
                try {
                    RequireOwnedAsset(path, ownedDirectory);
                    var source = PcmWave.Open(path);
                    var doc = File.Exists(path + ".edits") ? SampleDocument.Restore(source, path + ".edits") : new(source);
                    ChopAudioSettings.ValidateFile(source);
                    ValidateRetainedKit(path, doc.Slices().Length);
                    assets.Add(new(System.IO.Path.GetFileName(path), source.Frames / (double)source.SampleRate, doc.Slices().Length, path == currentPath));
                } catch (Exception e) when (e is IOException or ArgumentException or FormatException or OverflowException or UnauthorizedAccessException) { }
            }
            return (IReadOnlyList<RetainedAsset>)assets;
        });
    }
    private static void RequireOwnedAsset(string path, string root)
    {
        if (!string.Equals(System.IO.Path.GetDirectoryName(System.IO.Path.GetFullPath(path)), System.IO.Path.GetFullPath(root), StringComparison.OrdinalIgnoreCase)
            || (File.GetAttributes(path) & FileAttributes.ReparsePoint) != 0) throw new IOException("Asset must be an owned sample file.");
        foreach (var suffix in new[] { ".edits", ".kitsettings", ".chopaudio" })
            if (File.Exists(path + suffix) && (File.GetAttributes(path + suffix) & FileAttributes.ReparsePoint) != 0)
                throw new IOException("Sample settings must be owned files.");
    }
    private static void ValidateRetainedKit(string path, int chopCount)
    {
        var sidecar = path + ".kitsettings";
        if (!File.Exists(sidecar)) return;
        if (new FileInfo(sidecar).Length > 4096) throw new IOException("Oversized kit settings.");
        var lines = File.ReadAllLines(sidecar);
        if (lines.Length is not (6 or 7) || lines[0] != "SLOOP-KIT-1") throw new FormatException("Invalid kit settings.");
        var options = new KitSettings(System.Text.Encoding.UTF8.GetString(Convert.FromBase64String(lines[1])),
            Enum.Parse<SampleMapping>(lines[2]), int.Parse(lines[3], CultureInfo.InvariantCulture), Enum.Parse<MonoChoice>(lines[4]),
            double.Parse(lines[5], CultureInfo.InvariantCulture), lines.Length == 7 && bool.Parse(lines[6]));
        SampleKitPlan.Validate(options, chopCount);
    }
    public async Task ReuseAssetAsync(string name)
    {
        if (Busy || RecordingActive) return;
        Busy = true; StopPreview(); Status = "Opening retained sample…"; Notify();
        try {
            SessionStore.RequirePath(name);
            if (System.IO.Path.GetFileName(name) != name || !name.EndsWith(".wav", StringComparison.OrdinalIgnoreCase)) throw new IOException("Invalid sample asset name.");
            var path = System.IO.Path.Combine(directory, name);
            var result = await Task.Run(() => {
                RequireOwnedAsset(path, directory);
                var source = PcmWave.Open(path);
                ChopAudioSettings.ValidateFile(source);
                var doc = File.Exists(path + ".edits") ? SampleDocument.Restore(source, path + ".edits") : new(source);
                ValidateRetainedKit(path, doc.Slices().Length);
                return (Document: doc, Peaks: source.Peaks());
            });
            File.WriteAllText(Pointer + ".tmp", name); File.Move(Pointer + ".tmp", Pointer, true);
            Document = result.Document; Peaks = result.Peaks; Cursor = Document.Start;
            converted = null; conversionKey = null; PendingExport = null; PendingKitExport = null;
            tapReview = null; chopProposal = null; settingsSource = null;
            Status = "Retained sample opened with saved chops and settings.";
        } catch (Exception error) { Status = "Could not open retained sample: " + error.Message; }
        finally { Busy = false; Notify(); }
    }
    private TapChopReview? tapReview;
    private PcmWave? previewSource;
    private long previewStart, previewEnd;
    public TapChopReview? TapReview => tapReview?.IsCurrent(Document) == true ? tapReview : null;
    public bool CanTapPlayback => TapReview is not null && player?.IsPlaying == true && ReferenceEquals(previewSource, Document?.Source);
    public async Task StartTapChoppingAsync()
    {
        if (Busy || Document is not { } doc) return;
        tapReview = new(doc); chopProposal = null;
        await PreviewAsync(doc.Start, doc.End);
    }
    public void TapPlayback(int latencyMilliseconds = 0)
    {
        if (!CanTapPlayback) { Status = "Play the source to add tap markers."; Notify(); return; }
        if (TapReview!.AddPlaybackPosition(previewStart, previewEnd, player!.CurrentPosition, latencyMilliseconds))
            Status = $"{TapReview.Boundaries.Count} tap markers to review. Apply replaces chops.";
        else Status = "Tap ignored at an edge, duplicate position or 16-chop limit.";
        Notify();
    }
    public void UndoTap() { TapReview?.Undo(); Notify(); }
    public void DiscardTapReview() { StopPreview(); tapReview = null; Notify(); }
    public void ApplyTapReview()
    {
        if (TapReview is not { } review || review.Boundaries.Count == 0) return;
        Edit(review.Apply); tapReview = null;
    }
}
