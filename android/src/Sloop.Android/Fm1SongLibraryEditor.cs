using Android.Content;
using Android.Widget;
using Sloop.Workstation;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    const int ImportFm1SongRequest=418;
    private Fm1SongStore HardwareSongs=>new(System.IO.Path.Combine(FilesDir!.AbsolutePath,"fm1-songs"));
    private bool CanTransferSongs=>connection.Snapshot.Device is {ProtocolVersion:>=14,ParameterCount:71}&&
        !connection.Snapshot.Busy&&!connection.Snapshot.IsSimulated&&!connection.IsPlaying&&!connection.IsRecording&&!connection.Performer.Active;
    private void AddFm1SongLibrary()
    {
        content.AddView(Label("FM1 songs",18));
        content.AddView(Label("Keep named songs on your phone, beyond A–D. Includes all four sections, their song order, current project, user samples, sound banks and edited FM6 voices.",13));
        var save=new Button(this){Text="Save from FM1",Enabled=CanTransferSongs};
        save.Click+=(_,_)=>{
            var input=new EditText(this){Hint="Song name"};input.SetSingleLine(true);
            new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Save FM1 song")!.SetView(input)!
                .SetPositiveButton("Save",async(_,_)=>{
                    try {await connection.EditDeviceAsync("Saving FM1 song and its sounds…",async(c,t)=>{
                        var song=await Fm1Songs.Capture(c,connection.Snapshot.Device!,input.Text??"",t);HardwareSongs.Save(song);return true;});editing.SetStatus("FM1 song saved to your phone.");}
                    catch(Exception e){editing.SetStatus("Song save failed: "+e.Message);}ShowWorkspace();})!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
        };content.AddView(save);
        ActionButton("Import FM1 song",()=>{var intent=new Intent(Intent.ActionOpenDocument);intent.AddCategory(Intent.CategoryOpenable);intent.SetType("*/*");StartActivityForResult(intent,ImportFm1SongRequest);});
        if(!CanTransferSongs)content.AddView(Label("Connect the FM1, stop playback/recording and release notes to save or load songs.",13));
        try {
            foreach(var song in HardwareSongs.List().Where(s=>s.Name.Contains(libraryFilter,StringComparison.OrdinalIgnoreCase))) {
                content.AddView(Label($"{song.Name}\n{song.CreatedUtc.LocalDateTime:g}",14));
                var load=new Button(this){Text="Load onto FM1",Enabled=CanTransferSongs};
                load.Click+=(_,_)=>new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Load "+song.Name+"?")!
                    .SetMessage("Replaces the current project, A–D sections, song order, user samples and sound banks. Device preferences and MIDI mappings stay as they are. A recovery song is saved on your phone before any changes.")!
                    .SetPositiveButton("Load",async(_,_)=>{
                        try {
                            await connection.EditDeviceAsync("Backing up FM1, then loading song…",async(c,t)=>{
                                await Fm1Songs.Restore(c,connection.Snapshot.Device!,HardwareSongs.Read(song.Id),backup=>{HardwareSongs.Save(backup);return Task.CompletedTask;},t);return true;});
                            editing.SetStatus("Song loaded and read back successfully. Previous work is saved in FM1 songs.");
                            await connection.RefreshAfterSongLoadAsync();
                        } catch(Exception e){editing.SetStatus("Song load interrupted: "+e.Message+" Your recovery song remains in the Library. Reconnect before retrying.");}
                        ShowWorkspace();})!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();content.AddView(load);
                ActionButton("Export FM1 song",()=>editing.Run(()=>ExportBytes(Fm1Songs.Encode(HardwareSongs.Read(song.Id)),"song.fm1song","application/json"),"Choose song export destination."));
            }
        } catch(Exception e){content.AddView(Label("Could not read song library: "+e.Message,13));}
    }
    private async Task ImportFm1SongAsync(Result resultCode,Intent? data)
    {
        if(resultCode!=Result.Ok||data?.Data is not {} uri)return;
        try {await Task.Run(()=>{using var input=ContentResolver!.OpenInputStream(uri)??throw new IOException("Cannot open song.");var song=Fm1Songs.Decode(input);HardwareSongs.Save(song with{Id=Guid.NewGuid()});});editing.SetStatus("FM1 song imported. Hardware has not changed.");}
        catch(Exception e){editing.SetStatus("Song import failed: "+e.Message);}ShowWorkspace();
    }
}
