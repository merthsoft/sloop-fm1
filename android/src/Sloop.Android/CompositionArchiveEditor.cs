using Android.App;
using Android.Content;
using Android.Widget;
using System.Collections.Immutable;
using Sloop.Sequencing;
using Sloop.Workstation;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    const int CompositionImportRequest = 7411, CompositionExportRequest = 7412;
    AppPattern? compositionImportSource;
    string? compositionImportRoot;
    byte[]? compositionExportBytes;

    void ImportCompositionArchive(AppPattern source)
    {
        compositionImportSource = source;
        compositionImportRoot = EffectiveWorkspaceRoot;
        try {
            var intent = new Intent(Intent.ActionOpenDocument);
            intent.AddCategory(Intent.CategoryOpenable); intent.SetType("*/*");
            StartActivityForResult(intent, CompositionImportRequest);
        } catch(Exception e) { compositionImportSource = null; compositionImportRoot = null; editing.SetStatus("Draft import picker failed: " + e.Message); }
    }

    void ExportCompositionArchive(CompositionDraft draft, ImmutableArray<CompositionPart> kept, string? name = null)
    {
        try {
            compositionExportBytes = CompositionDraftArchive.Export(new(Guid.NewGuid(), name ?? $"Composition seed {draft.Intent.Seed}", draft, kept));
            var intent = new Intent(Intent.ActionCreateDocument);
            intent.AddCategory(Intent.CategoryOpenable); intent.SetType("application/zip");
            intent.PutExtra(Intent.ExtraTitle, $"composition-{draft.Intent.Seed}.sloopdraft");
            StartActivityForResult(intent, CompositionExportRequest);
        } catch(Exception e) { compositionExportBytes = null; editing.SetStatus("Draft export failed: " + e.Message); }
    }

    // Parent FileResultAsync must await this method before its own dispatch.
    async Task<bool> TryHandleCompositionFileResultAsync(int requestCode, Result resultCode, Intent? data)
    {
        if(requestCode != CompositionImportRequest && requestCode != CompositionExportRequest) return false;
        var source = compositionImportSource; var bytes = compositionExportBytes;
        var root = compositionImportRoot;
        if(requestCode == CompositionImportRequest) { compositionImportSource = null; compositionImportRoot = null; }
        else compositionExportBytes = null;
        if(resultCode != Result.Ok || data?.Data is not {} uri) return true;
        try {
            if(requestCode == CompositionExportRequest) {
                if(bytes is null) throw new InvalidDataException("Export expired; choose Export again.");
                await Task.Run(() => {
                    using var output = ContentResolver!.OpenOutputStream(uri, "wt") ?? throw new IOException("Cannot open export destination.");
                    output.Write(bytes); output.Flush();
                });
                editing.SetStatus("Portable composition draft exported.");
            } else {
                if(source is null || root is null) throw new InvalidDataException("Import expired; choose Import again.");
                void RequireCurrentSource() {
                    if(IsFinishing || IsDestroyed || root != EffectiveWorkspaceRoot || !ReferenceEquals(editing.Sequence.Current, source))
                        throw new InvalidOperationException("The workspace or sequence changed during import. Choose Import again to review against the current sequence.");
                }
                RequireCurrentSource();
                var imported = await Task.Run(() => {
                    using var input = ContentResolver!.OpenInputStream(uri) ?? throw new IOException("Cannot open draft archive.");
                    return CompositionDraftArchive.Import(input);
                });
                if(IsFinishing || IsDestroyed) return true;
                RequireCurrentSource();
                var draft = imported.Draft;
                var summary = new ScrollView(this);
                summary.AddView(Label($"{imported.Name}\nGenerator: {draft.Generator}\nPrompt: {draft.Prompt}\nSeed {draft.Intent.Seed} · {draft.Intent.Bars} bars · {draft.Intent.Tempo} BPM\n{draft.Intent.Progression} · {draft.Intent.Rhythm} · {draft.Intent.Density}\nParts: {string.Join(", ", draft.Intent.Parts)}\nKeep: {string.Join(", ", imported.Kept)}\n{draft.Pattern.Notes.Length} exact editable notes · PPQ {draft.Pattern.TicksPerQuarter}\n\nImport saves a new local copy and opens draft review. The current sequence changes only through Review apply and Apply locally.",14));
                new AlertDialog.Builder(this)!.SetTitle("Review imported composition")!.SetView(summary)!
                    .SetNegativeButton("Cancel",(_,_)=>{})!
                    .SetPositiveButton("Import and open draft",(_,_)=> {
                        try {
                            RequireCurrentSource();
                            var saved = CompositionDraftArchive.Publish(CompositionCatalogPath, imported);
                            StopCompositionAudition();
                            ReviewCompositionDraft(source, saved.Draft, saved.Kept.ToImmutableHashSet());
                            editing.SetStatus("Imported a new saved draft. Current sequence unchanged.");
                        } catch(Exception e) { editing.SetStatus("Draft import failed; existing drafts retained: " + e.Message); }
                    })!.Show();
            }
        } catch(Exception e) { editing.SetStatus("Draft archive operation failed: " + e.Message); }
        return true;
    }
}
