using Android.Widget;
using System.Collections.Immutable;
using Sloop.Sequencing;
using Sloop.Workstation;
namespace Sloop.Android;
public sealed partial class MainActivity
{
    CompositionAudition? compositionAudition;
    string CompositionCatalogPath=>System.IO.Path.Combine(FilesDir!.AbsolutePath,"composition-drafts.json");
    // Host lifecycle/workspace/session transitions may call this; only our transport epoch is stopped.
    void StopCompositionAudition()
    {
        compositionAudition?.Stop();
    }
    async Task AuditionCompositionDraft(CompositionDraft draft)
    {
        OfflineComposition.ValidateDraft(draft);
        var pattern=draft.Pattern;
        if(connection.Snapshot.IsGenericMidi)pattern=pattern with {Notes=pattern.Notes.Select(n=>n with {Channel=n.Channel switch {0=>genericSequenceChannels[0],1=>genericSequenceChannels[1],2=>genericSequenceChannels[2],9=>genericSequenceChannels[3],_=>n.Channel}}).ToImmutableArray()};
        compositionAudition??=new(()=>connection.TransportEpoch,()=>connection.StopPlaying());
        await compositionAudition.RunAsync(()=>connection.LoopPatternAsync(pattern,draft.Intent.Tempo));
    }
    void SaveCompositionDraft(CompositionDraft draft,ImmutableArray<CompositionPart> kept)
    {
        var name=new EditText(this){Hint="Draft name (1..80 characters)",Text=$"{PerformanceHarmony.Names[draft.Intent.Key]} {draft.Intent.Scale} · seed {draft.Intent.Seed}"};
        new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Save composition draft")!.SetView(name)!
            .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Save",(_,_)=> {
                try {
                    var catalog=CompositionDraftStore.Load(CompositionCatalogPath);
                    var entry=new SavedCompositionDraft(Guid.NewGuid(),(name.Text??"").Trim(),draft,kept);
                    CompositionDraftStore.Save(CompositionCatalogPath,catalog with {Drafts=catalog.Drafts.Add(entry)});
                    editing.SetStatus("Draft saved on this phone. Current sequence unchanged.");
                }catch(Exception e){editing.SetStatus("Draft save failed; existing catalog retained: "+e.Message);}
            })!.Show();
    }
    void OpenCompositionDrafts(AppPattern source)
    {
        try {
            var catalog=CompositionDraftStore.Load(CompositionCatalogPath);
            new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Saved composition drafts")!
                .SetItems(catalog.Drafts.Select(d=>$"{d.Name} · {d.Draft.Intent.Tempo} BPM").ToArray(),(_,a)=> {
                    var saved=catalog.Drafts[a.Which];
                    new global::Android.App.AlertDialog.Builder(this)!.SetTitle(saved.Name)!
                        .SetMessage("Opening restores the editable draft and keep-part choices. Keep applies through a separate reviewed undo step.")!
                        .SetNegativeButton("Back",(_,_)=>OpenCompositionDrafts(source))!
                        .SetNeutralButton("More…",(_,_)=> {
                            new global::Android.App.AlertDialog.Builder(this)!.SetTitle(saved.Name)!
                            .SetItems(new[]{"Export portable draft…","Delete saved draft"},(_,choice)=> {
                            if(choice.Which==0){ExportCompositionArchive(saved.Draft,saved.Kept,saved.Name);return;}
                            try{var latest=CompositionDraftStore.Load(CompositionCatalogPath);CompositionDraftStore.Save(CompositionCatalogPath,latest with {Drafts=latest.Drafts.Where(d=>d.Id!=saved.Id).ToImmutableArray()});editing.SetStatus("Saved draft deleted.");}
                            catch(Exception e){editing.SetStatus(e.Message);}
                            })!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
                        })!.SetPositiveButton("Open draft",(_,_)=>ReviewCompositionDraft(source,saved.Draft,saved.Kept.ToImmutableHashSet()))!.Show();
                })!.SetNeutralButton("Import portable draft…",(_,_)=>ImportCompositionArchive(source))!.SetNegativeButton("Close",(_,_)=>{})!.Show();
        }catch(Exception e){editing.SetStatus("Saved drafts could not be opened; file retained: "+e.Message);}
    }
}
