using Android.Widget;
using Sloop.Android.Services;
using Sloop.Protocol;
using Sloop.SoundDesign;
using Sloop.Workstation;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    private void AddSoundBankEditor(int track,SoundDocument doc)
    {
        Section("sound.bank","FM1 bank · persistent base voices",()=> {
            content.AddView(Label("Store the local base voice in B1–B27 flash. Macros remain track settings. To use it in a project, select that bank PTCH on FM1 and save the project separately.",14));
            bool ready=!connection.Snapshot.Busy&&!connection.IsPlaying&&!connection.Snapshot.IsSimulated&&
                connection.Snapshot.Device?.ProtocolVersion>=9&&!Enumerable.Range(0,3).Any(t=>soundAuditions[t]?.Active==true);
            if(Enumerable.Range(0,3).Any(t=>soundAuditions[t]?.Active==true))
                content.AddView(Label("Finish or abandon retained A/B auditions before using the shared FM1 bank.",14));
            AsyncButton("Inspect / load FM1 bank voice locally…",async()=> {
                var entries=await connection.ListSoundBankAsync();
                new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Read bank · replaces local base voice")!
                    .SetItems(entries.Select(e=>$"B{e.Slot+1} · {(e.Used?e.Name:"empty")}").ToArray(),async(_,a)=> {
                        try {
                            var original=editing.Sounds[track];long revision=original.Revision;string id=original.TargetId;
                            var voice=await connection.ReadSoundBankAsync(entries[a.Which].Slot);
                            if(voice is null)throw new IOException("Bank slot is empty or unavailable.");
                            if(state.SelectedTrack!=track||editing.Sounds[track].TargetId!=id||editing.Sounds[track].Revision!=revision||soundAuditions[track]?.Active==true)
                                throw new IOException("Local track or audition changed during bank read. Read again.");
                            editing.ChangeSound(track,original.State with{Patch=voice},"FM1 bank read");
                            editing.SetStatus("Bank base voice read locally; macros preserved. Undo restores the previous local voice.");
                        }catch(Exception e){editing.SetStatus(e.Message);}
                    })!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
            },ready);
            AsyncButton("Choose FM1 bank destination…",async()=> {
                var entries=await connection.ListSoundBankAsync();
                new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Store local voice · choose B slot")!
                    .SetItems(entries.Select(e=>$"B{e.Slot+1} · {(e.Used?e.Name:"empty")}").ToArray(),async(_,a)=> {
                        try {await Prepare(entries[a.Which].Slot);}catch(Exception e){editing.SetStatus(e.Message);}
                    })!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
            },ready);
            async Task Prepare(byte slot) {
                var source=editing.Sounds[track]; string id=source.TargetId;long revision=source.Revision;
                var entry=new EditText(this){Text=source.State.Patch.Name,Hint="1–10 ASCII characters"};
                new global::Android.App.AlertDialog.Builder(this)!.SetTitle($"B{slot+1} · saved voice name")!.SetView(entry)!
                    .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Review save",async(_,_)=> {
                        try {
                            Guard(); var prepared=await connection.PrepareSoundBankSaveAsync(slot,source.State.Patch,entry.Text??"");Guard();
                            string previous=prepared.Plan.Before is null?"empty":PatchCodec.Unpack(prepared.Plan.Before).Name;
                            new global::Android.App.AlertDialog.Builder(this)!.SetTitle(prepared.Plan.Before is null?"Confirm flash save":"Confirm overwrite")!
                                .SetMessage($"Synth {track+1} local voice → B{slot+1}\nDestination: {previous}\nNew name: {entry.Text}\nBase voice only; track macros and project are separate.")!
                                .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton(prepared.Plan.Before is null?"Save to flash":"Overwrite B"+(slot+1),async(_,_)=> {
                                    try {Guard();var result=await connection.SaveSoundBankAsync(prepared,true,Guard);editing.SetStatus(result.Message);}
                                    catch(Exception e){editing.SetStatus(e.Message);} })!.Show();
                        } catch(Exception e){editing.SetStatus(e.Message);}
                    })!.Show();
                void Guard() {
                    var current=editing.Sounds[track];
                    if(state.SelectedTrack!=track||current.TargetId!=id||current.Revision!=revision||Enumerable.Range(0,3).Any(t=>soundAuditions[t]?.Active==true))
                        throw new IOException("Track, local sound or A/B ownership changed. Choose the destination again.");
                }
                await Task.CompletedTask;
            }
        });
    }
}
