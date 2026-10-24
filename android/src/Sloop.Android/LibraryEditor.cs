using Android.Content;
using Android.Widget;
using Sloop.SoundDesign;
using Sloop.Workstation;
using Sloop.Android.Services;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    private const int ImportSoundRequest=414, ExportFileRequest=415, ImportSessionRequest=416;
    private string libraryFilter="";
    private SessionWorkspace SessionLibrary=>new(FilesDir!.AbsolutePath,EffectiveWorkspaceRoot);
    // Root supplies one transactional adapter: stop notes, stage documents, adopt all editors.
    public Func<SessionCandidate,Task>? AdoptSessionAsync {get;set;}
    private void LibraryName(string title,Action<string> action)
    {
        var input=new EditText(this){Hint="Name"};input.SetSingleLine(true);
        new global::Android.App.AlertDialog.Builder(this)!.SetTitle(title)!.SetView(input)!
            .SetPositiveButton("Save",(_,_)=>{editing.Run(()=>action(input.Text??""),"Saved to Library.");ShowWorkspace();})!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
    }
    private void ExportSession(Guid id)
    {
        StageFileExport("session.sloopzip","application/zip",output=>SessionLibrary.Store.Export(id,output));
    }
    private void ExportBytes(byte[] bytes,string name,string type="application/octet-stream")
    {
        StageFileExport(name,type,output=>output.Write(bytes));
    }
    private void ImportSound(int track)
    {
        if(soundImportRequest is {} old)FileRequests.Complete(old);
        soundImportRequest=FileRequests.BeginImport(EffectiveWorkspaceRoot,track).Id;
        try {
            var intent=new Intent(Intent.ActionOpenDocument);intent.AddCategory(Intent.CategoryOpenable);intent.SetType("*/*");StartActivityForResult(intent,ImportSoundRequest);
        }catch {FileRequests.Complete(soundImportRequest.Value);soundImportRequest=null;throw;}
    }
    private async Task FileResultAsync(int requestCode,Result resultCode,Intent? data)
    {
        if(requestCode==ImportMidiRequest){await ImportMidiFileResult(resultCode,data);return;}
        if(requestCode==ImportFm1SongRequest){await ImportFm1SongAsync(resultCode,data);return;}
        if (await TryHandleCompositionFileResultAsync(requestCode, resultCode, data)) return;
        if(requestCode==ExportFileRequest) {
            var id=fileExportRequest;bool prepared=false;
            try {
                if(resultCode!=Result.Ok||data?.Data is not {} target){if(id is {} cancelled)FinishFileExport(cancelled);return;}
                if(id is null)throw new IOException("Export expired. Choose Export again.");
                FileRequests.RequireExport(id.Value);
                prepared=true;
                await Task.Run(()=>{using var output=ContentResolver!.OpenOutputStream(target,"wt")??throw new IOException("Cannot write destination.");FileRequests.CopyExport(id.Value,output);});
                FinishFileExport(id.Value);editing.SetStatus("File exported.");
            }catch(Exception e){
                editing.SetStatus((prepared?"Export failed; prepared file retained. Retry in Library: ":"Export failed; choose Export again: ")+e.Message);
                if(id is {} retry){if(prepared)OfferExportRetry(retry);else FinishFileExport(retry);}
            }
            return;
        }
        if(requestCode==ImportSessionRequest){
            if(resultCode!=Result.Ok||data?.Data is not {} source)return;
            try{var candidate=await Task.Run(()=>{using var input=ContentResolver!.OpenInputStream(source)??throw new IOException("Cannot open archive.");return SessionLibrary.Import(input);});editing.SetStatus("Imported session: "+candidate.Manifest.Name+". Current work preserved.");}
            catch(Exception e){editing.SetStatus("Session import failed: "+e.Message);}ShowWorkspace();return;
        }
        if(requestCode!=ImportSoundRequest)return;
        var importId=soundImportRequest;
        if(resultCode!=Result.Ok||data?.Data is not {} uri){if(importId is {} cancelled)FileRequests.Complete(cancelled);soundImportRequest=null;return;}
        try {
            if(importId is null)throw new IOException("Import expired. Choose Import again.");
            var request=FileRequests.RequireImport(importId.Value,EffectiveWorkspaceRoot);
            void RequireDestination() {
                if(activityDestroyed||IsFinishing||IsDestroyed||soundImportRequest!=request.Id)
                    throw new IOException("Import review expired. Choose Import again.");
                FileRequests.RequireImport(request.Id,EffectiveWorkspaceRoot);
            }
            var asset=await Task.Run(()=> {
                using var input=ContentResolver!.OpenInputStream(uri)??throw new IOException("Cannot open patch.");
                using var buffer=new MemoryStream();var chunk=new byte[4096];int count;
                while((count=input.Read(chunk))>0) {if(buffer.Length+count>4104)throw new IOException("Choose one complete single-voice or 32-voice DX bank SysEx.");buffer.Write(chunk,0,count);}
                return SysExCodec.Import(buffer.ToArray());
            });
            RequireDestination();int track=request.Track;
            var originals=System.IO.Path.Combine(request.Workspace,"patch-imports");Directory.CreateDirectory(originals);
            File.WriteAllBytes(System.IO.Path.Combine(originals,Guid.NewGuid().ToString("N")+".syx"),asset.ExportOriginal());
            void Choose(int index)=>editing.Run(()=> {
                RequireDestination();editing.ChangeSound(track,editing.Sounds[track].State with{Patch=asset.Voices[index]},"SysEx import");
                FileRequests.Complete(request.Id);soundImportRequest=null;
            },"Imported patch saved; original SysEx retained.");
            if(asset.Voices.Length==1)Choose(0);
            else {
                var dialog=new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Choose voice from bank")!.SetItems(asset.Voices.Select(p=>p.Name).ToArray(),(_,a)=>Choose(a.Which))!.Create()!;
                dialog.DismissEvent+=(_,_)=>{FileRequests.Complete(request.Id);if(soundImportRequest==request.Id)soundImportRequest=null;};dialog.Show();
            }
        }catch(Exception e){if(importId is {} failed)FileRequests.Complete(failed);if(soundImportRequest==importId)soundImportRequest=null;editing.SetStatus("Patch import failed: "+e.Message);}
    }
    private void AddLibraryEditor()
    {
        content.AddView(Label(editing.Status,14));
        if(fileExportRequest is {} retry) {
            ActionButton("Retry prepared export…",LaunchFileExport);
            ActionButton("Discard prepared export",()=>FinishFileExport(retry));
        }
        AddFm1SongLibrary();
        ActionButton("Browse sample library", BrowseCrossSessionSamples);
        content.AddView(Label("Sessions & performance presets",18));
        ActionButton("Save named session",()=>LibraryName("Save session",name=>{if(SampleWorkspace.Get(this).Busy)throw new InvalidOperationException("Wait for sample work to finish.");
            connection.StopPlaying();
            SaveSessionSettings();
            SessionLibrary.Save(name,scenes:CaptureSessionSceneIds?.Invoke(),presets:SessionLibrary.Presets().Select(p=>p.Id).ToArray(),extraAssets:CaptureSessionExtraAssets?.Invoke());}));
        ActionButton("Import session archive",()=>{var intent=new Intent(Intent.ActionOpenDocument);intent.AddCategory(Intent.CategoryOpenable);intent.SetType("*/*");StartActivityForResult(intent,ImportSessionRequest);});
        ActionButton("Save performance preset",()=>LibraryName("Save performance preset",name=>SessionLibrary.Store.SavePreset(name,SessionPresetCodec.Encode(performOptions))));
        ActionButton("Filter Library",()=>{var input=new EditText(this){Text=libraryFilter,Hint="Name contains"};new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Filter Library")!.SetView(input)!.SetPositiveButton("Apply",(_,_)=>{libraryFilter=input.Text??"";ShowWorkspace();})!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();});
        foreach(var session in SessionLibrary.Store.List().Where(s=>s.Name.Contains(libraryFilter,StringComparison.OrdinalIgnoreCase)).Take(40)){
            content.AddView(Label($"{session.Name} · {session.Assets.Length} assets\n{session.CreatedUtc.LocalDateTime:g}",14));
            AsyncButton("Load session",async()=>{try{var candidate=SessionLibrary.Prepare(session.Id);if(AdoptSessionAsync is not {} adopt)throw new InvalidOperationException("Session adoption adapter is not connected.");await adopt(candidate);editing.SetStatus("Loaded "+session.Name);}finally{ShowWorkspace();}},AdoptSessionAsync is not null);
            ActionButton("Export archive",()=>editing.Run(()=>ExportSession(session.Id),"Choose archive destination."));
        }
        foreach(var preset in SessionLibrary.Presets().Where(p=>p.Name.Contains(libraryFilter,StringComparison.OrdinalIgnoreCase)).Take(40)){
            ActionButton("Preset · "+preset.Name,()=>editing.Run(()=>{var options=SessionPresetCodec.Decode(preset.Settings);StopPerformance();performOptions=options;ShowWorkspace();},"Performance preset loaded."));
        }
        content.AddView(Label("Saved work & recovery",18));
        var directory=EffectiveWorkspaceRoot;
        var paths=new[]{"workspaces","patch-imports","slot-backups"}.SelectMany(d=>{var root=d=="slot-backups"?FilesDir!.AbsolutePath:directory;return Directory.Exists(System.IO.Path.Combine(root,d))?Directory.GetFiles(System.IO.Path.Combine(root,d)):[];})
            .Where(p=>!p.EndsWith(".tmp")&&System.IO.Path.GetFileName(p).Contains(libraryFilter,StringComparison.OrdinalIgnoreCase)).OrderByDescending(File.GetLastWriteTimeUtc).Take(40).ToArray();
        foreach(var (folder,title) in new[]{("workspaces","Patches, patterns & proposals"),("slot-backups","FM1 slot backups & recovery"),("patch-imports","Original SysEx imports")}) {
        var group=paths.Where(p=>System.IO.Path.GetFileName(System.IO.Path.GetDirectoryName(p))==folder).ToArray();
        Section("library."+folder,$"{title} · {group.Length}",()=> {
        foreach(var path in group) {
            string selected=path;
            var file=System.IO.Path.GetFileName(path);
            var kind=file.EndsWith(".before.fm1")?"Previous slot backup":file.EndsWith(".replacement.fm1")?"Replacement slot image":file.EndsWith(".transfer")?"Transfer record":file.EndsWith(".syx")?"Original DX import":file.StartsWith("proposal-")?"Sound proposal":file;
            content.AddView(Label($"{kind}\n{File.GetLastWriteTime(path):g}",14));
            ActionButton("Export file",()=>ExportBytes(File.ReadAllBytes(selected),System.IO.Path.GetFileName(selected)));
            if(path.EndsWith(".before.fm1")||path.EndsWith(".replacement.fm1"))AddRestoreSlotButton(selected);
        }
        },folder=="workspaces");
        }
        content.AddView(Label("Sound and sequence content survives restart; undo/redo is session history. Transfer backups and pending-transfer records remain here after interruption.",14));
    }
}
