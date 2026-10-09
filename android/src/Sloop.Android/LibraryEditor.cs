using Android.Content;
using Android.Widget;
using Sloop.SoundDesign;
using Sloop.Workstation;
using Sloop.Android.Services;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    private const int ImportSoundRequest=414, ExportFileRequest=415, ImportSessionRequest=416;
    private string? sessionExportPath;
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
        var path=System.IO.Path.Combine(CacheDir!.AbsolutePath,"session-"+Guid.NewGuid().ToString("N")+".sloopzip");
        try{using(var output=File.Create(path))SessionLibrary.Store.Export(id,output);sessionExportPath=path;exportBytes=null;
            var intent=new Intent(Intent.ActionCreateDocument);intent.AddCategory(Intent.CategoryOpenable);intent.SetType("application/zip");intent.PutExtra(Intent.ExtraTitle,"session.sloopzip");StartActivityForResult(intent,ExportFileRequest);
        }catch{if(File.Exists(path))File.Delete(path);throw;}
    }
    private byte[]? exportBytes;
    private int importSoundTrack;
    private void ExportBytes(byte[] bytes,string name,string type="application/octet-stream")
    {
        exportBytes=bytes.ToArray();
        sessionExportPath=null;
        var intent=new Intent(Intent.ActionCreateDocument);intent.AddCategory(Intent.CategoryOpenable);intent.SetType(type);intent.PutExtra(Intent.ExtraTitle,name);
        StartActivityForResult(intent,ExportFileRequest);
    }
    private void ImportSound(int track)
    {
        importSoundTrack=track;
        var intent=new Intent(Intent.ActionOpenDocument);intent.AddCategory(Intent.CategoryOpenable);intent.SetType("*/*");StartActivityForResult(intent,ImportSoundRequest);
    }
    private async Task FileResultAsync(int requestCode,Result resultCode,Intent? data)
    {
        if (await TryHandleCompositionFileResultAsync(requestCode, resultCode, data)) return;
        if(requestCode==ExportFileRequest) {
            var bytes=exportBytes;exportBytes=null;var path=sessionExportPath;sessionExportPath=null;
            try{if(resultCode!=Result.Ok||data?.Data is not {} target||bytes is null&&path is null)return;
                await Task.Run(()=>{using var output=ContentResolver!.OpenOutputStream(target,"wt")??throw new IOException("Cannot write destination.");if(path is not null){using var input=File.OpenRead(path);input.CopyTo(output);}else output.Write(bytes!);});editing.SetStatus("File exported.");}
            catch(Exception e){editing.SetStatus("Export failed: "+e.Message);}finally{if(path is not null&&File.Exists(path))File.Delete(path);}return;
        }
        if(requestCode==ImportSessionRequest){
            if(resultCode!=Result.Ok||data?.Data is not {} source)return;
            try{var candidate=await Task.Run(()=>{using var input=ContentResolver!.OpenInputStream(source)??throw new IOException("Cannot open archive.");return SessionLibrary.Import(input);});editing.SetStatus("Imported session: "+candidate.Manifest.Name+". Current work preserved.");}
            catch(Exception e){editing.SetStatus("Session import failed: "+e.Message);}ShowWorkspace();return;
        }
        if(requestCode!=ImportSoundRequest||resultCode!=Result.Ok||data?.Data is not {} uri)return;
        try {
            var asset=await Task.Run(()=> {
                using var input=ContentResolver!.OpenInputStream(uri)??throw new IOException("Cannot open patch.");
                using var buffer=new MemoryStream();var chunk=new byte[4096];int count;
                while((count=input.Read(chunk))>0) {if(buffer.Length+count>4104)throw new IOException("Choose one complete single-voice or 32-voice DX bank SysEx.");buffer.Write(chunk,0,count);}
                return SysExCodec.Import(buffer.ToArray());
            });
            int track=importSoundTrack;
            var originals=System.IO.Path.Combine(EffectiveWorkspaceRoot,"patch-imports");Directory.CreateDirectory(originals);
            File.WriteAllBytes(System.IO.Path.Combine(originals,Guid.NewGuid().ToString("N")+".syx"),asset.ExportOriginal());
            void Choose(int index)=>editing.Run(()=>editing.ChangeSound(track,editing.Sounds[track].State with{Patch=asset.Voices[index]},"SysEx import"),"Imported patch saved; original SysEx retained.");
            if(asset.Voices.Length==1)Choose(0);
            else new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Choose voice from bank")!.SetItems(asset.Voices.Select(p=>p.Name).ToArray(),(_,a)=>Choose(a.Which))!.Show();
        }catch(Exception e){editing.SetStatus("Patch import failed: "+e.Message);}
    }
    private void AddLibraryEditor()
    {
        content.AddView(Label(editing.Status,14));
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
