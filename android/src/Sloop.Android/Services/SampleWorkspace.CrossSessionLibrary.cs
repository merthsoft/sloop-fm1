using Sloop.Workstation;

namespace Sloop.Android.Services;

public sealed partial class SampleWorkspace
{
    private SampleLibrary Library => new(System.IO.Path.Combine(context.FilesDir!.AbsolutePath, "sessions"));
    public Task<SampleLibraryIndex> CrossSessionSamplesAsync(CancellationToken token = default) => Library.IndexAsync(token);
    public async Task ReuseLibrarySampleAsync(LibrarySample asset, CancellationToken token = default)
    {
        if (Busy || RecordingActive) return;
        var ownedDirectory = directory;
        Busy = true; StopPreview(); Status = "Copying library sample into this session…"; Notify();
        try {
            var name = await Library.CopyAsync(asset, ownedDirectory, token);
            var path = System.IO.Path.Combine(ownedDirectory, name);
            var prepared = await Task.Run(() => {
                token.ThrowIfCancellationRequested(); var document = SampleLibrary.Validate(path);
                var peaks = Sloop.Core.Sampling.SamplingTools.Peaks(document.Source, 0, document.Source.Frames, token: token); token.ThrowIfCancellationRequested();
                return (Document: document, Peaks: peaks, Options: SampleLibrary.ReadKit(path, document.Slices().Length));
            }, token);
            token.ThrowIfCancellationRequested();
            if (directory != ownedDirectory) throw new InvalidOperationException("Session changed while copying. Browse again in the active session.");
            var temp = Pointer + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try {
                using (var output = new FileStream(temp, FileMode.CreateNew, FileAccess.Write)) {
                    output.Write(System.Text.Encoding.UTF8.GetBytes(name)); output.Flush(true);
                }
                token.ThrowIfCancellationRequested(); File.Move(temp, Pointer, true);
            } finally { if (File.Exists(temp)) File.Delete(temp); }
            Document = prepared.Document; Peaks = prepared.Peaks; Cursor = Document.Start;
            kitSettings = prepared.Options; settingsSource = path;
            converted = null; conversionKey = null; PendingExport = null; PendingKitExport = null;
            tapReview = null; chopProposal = null;
            Status = $"Copied from {asset.SessionName}; saved chops and audio settings restored. Prepare kit again.";
        } catch (OperationCanceledException) { Status = "Library reuse cancelled; current sample retained."; }
        catch (Exception error) { Status = "Could not reuse library sample: " + error.Message; }
        finally { Busy = false; Notify(); }
    }
}
