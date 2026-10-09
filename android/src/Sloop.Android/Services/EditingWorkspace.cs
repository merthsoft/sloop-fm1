using Android.Content;
using Sloop.Sequencing;
using Sloop.SoundDesign;
using Sloop.Workstation;

namespace Sloop.Android.Services;
public sealed class EditingWorkspace
{
    static EditingWorkspace? instance;
    public static EditingWorkspace Get(Context context){SessionWorkspace.RecoverStartup(context.FilesDir!.AbsolutePath);return instance??=new(context.ApplicationContext!);}
    string directory;
    public SoundDocument[] Sounds {get;}=new SoundDocument[3];
    public EditHistory Sequence {get;private set;}
    public EditHistory? Native {get;private set;}
    public string Status {get;private set;}="Local edits save on this phone.";
    public PatchDraft?[] Drafts {get;}=new PatchDraft?[3];
    public string Prompt {get;set;}="darker but keep the attack and tuning";
    public int Operator {get;set;}=1;
    public int Step {get;set;}
    public int Pitch {get;set;}=60;
    public int Velocity {get;set;}=100;
    public int Duration {get;set;}=240;
    public int Tempo {get;set;}=100;
    public SoundFamily Family {get;set;}=SoundFamily.Keys;
    public PatchLocks Locks {get;set;}=PatchLocks.None;
    public event Action? Changed;
    string SoundPath(int track)=>System.IO.Path.Combine(directory,$"sound-{track}.sloop");
    string PatternPath=>System.IO.Path.Combine(directory,"pattern.sloop");
    string NativePath=>System.IO.Path.Combine(directory,"hardware.sloop");
    EditingWorkspace(Context context)
    {
        directory=System.IO.Path.Combine(new WorkspaceGenerationStore(context.FilesDir!.AbsolutePath).ActiveRoot,"workspaces"); Directory.CreateDirectory(directory);
        for(int t=0;t<3;t++) {
            var target=$"local/track:{t}";
            try { Sounds[t]=File.Exists(SoundPath(t))?WorkspaceFiles.LoadSound(SoundPath(t),target):new(target,new(FactoryLibrary.Get(0),MacroContext.Neutral)); }
            catch(Exception e) { PreserveInvalid(SoundPath(t)); Sounds[t]=new(target,new(FactoryLibrary.Get(0),MacroContext.Neutral)); Status="Sound restore failed; original retained: "+e.Message; }
        }
        try { Sequence=new(File.Exists(PatternPath)?WorkspaceFiles.LoadPattern(PatternPath):new AppPattern(Guid.NewGuid(),Guid.NewGuid(),960,new(15360),[])); }
        catch(Exception e) { PreserveInvalid(PatternPath); Sequence=new(new AppPattern(Guid.NewGuid(),Guid.NewGuid(),960,new(15360),[])); Status="Pattern restore failed; original retained: "+e.Message; }
        try{if(File.Exists(NativePath))Native=new(WorkspaceFiles.LoadHardware(NativePath));}catch(Exception e){PreserveInvalid(NativePath);Status="Native restore failed; original retained: "+e.Message;}
    }
    void PreserveInvalid(string path) { if(File.Exists(path)) File.Copy(path,path+".invalid-"+DateTime.UtcNow.Ticks); }
    public sealed record Prepared(string Directory,SoundDocument[] Sounds,EditHistory Sequence,EditHistory? Native);
    public static Prepared Prepare(string root)
    {
        var directory=System.IO.Path.Combine(root,"workspaces");Directory.CreateDirectory(directory);
        var sounds=Enumerable.Range(0,3).Select(t=>{
            var path=System.IO.Path.Combine(directory,$"sound-{t}.sloop");
            return File.Exists(path)?WorkspaceFiles.LoadSound(path,$"local/track:{t}"):new SoundDocument($"local/track:{t}",new(FactoryLibrary.Get(0),MacroContext.Neutral));
        }).ToArray();
        var pattern=System.IO.Path.Combine(directory,"pattern.sloop");var native=System.IO.Path.Combine(directory,"hardware.sloop");
        return new(directory,sounds,new(File.Exists(pattern)?WorkspaceFiles.LoadPattern(pattern):new AppPattern(Guid.NewGuid(),Guid.NewGuid(),960,new(15360),[])),File.Exists(native)?new(WorkspaceFiles.LoadHardware(native)):null);
    }
    public Prepared Capture()=>new(directory,Sounds.ToArray(),Sequence,Native);
    public void Adopt(Prepared prepared)
    {
        directory=prepared.Directory;Array.Copy(prepared.Sounds,Sounds,3);Sequence=prepared.Sequence;Native=prepared.Native;Array.Clear(Drafts);
        Status="Workspace loaded; previous generation retained.";
    }
    public void SaveCurrent()
    {
        for(int t=0;t<3;t++)WorkspaceFiles.SaveSound(SoundPath(t),Sounds[t]);
        WorkspaceFiles.SavePattern(PatternPath,(AppPattern)Sequence.Current);
        if(Native is not null)WorkspaceFiles.SaveHardware(NativePath,(HardwarePattern)Native.Current);
    }
    public void Run(Action action,string success)
    {
        try { action(); Status=success; } catch(Exception e) { Status=e.Message; }
        Changed?.Invoke();
    }
    public void SetStatus(string text) {Status=text; Changed?.Invoke();}
    public void ChangeSound(int track,SoundState after,string description)
    {
        var doc=Sounds[track]; PatchValidation.Require(after.Patch); after.Macros.Validate();
        if(PatchCodec.ContentEquals(doc.State.Patch,after.Patch)&&doc.State.Macros==after.Macros) return;
        var draft=ProceduralDesigner.Refine(doc.TargetId,doc.Revision,doc.State,new(Family,42),[])
            with {After=after,Changes=ProceduralDesigner.Diff(doc.State.Patch,after.Patch),Prompt=description};
        var next=doc.Fork();next.Apply(draft);WorkspaceFiles.SaveSound(SoundPath(track),next);Sounds[track]=next;Drafts[track]=null;
    }
    public void ApplyDraft(int track)
    {
        var draft=Drafts[track]??throw new InvalidOperationException("No sound proposal.");
        if(!draft.IsNoOp) {var next=Sounds[track].Fork();next.Apply(draft);WorkspaceFiles.SaveSound(SoundPath(track),next);Sounds[track]=next;}
        Drafts[track]=null;
    }
    public void SoundHistory(int track,bool redo)
    {
        var next=Sounds[track].Fork();if(redo)next.Redo();else next.Undo();
        WorkspaceFiles.SaveSound(SoundPath(track),next);Sounds[track]=next;Drafts[track]=null;
    }
    public void EditPattern(EditProposal proposal) {var next=Sequence.Fork();next.Apply(proposal);WorkspaceFiles.SavePattern(PatternPath,(AppPattern)next.Current);Sequence=next;}
    public void PatternHistory(bool redo) {var next=Sequence.Fork();if(redo)next.Redo();else next.Undo();WorkspaceFiles.SavePattern(PatternPath,(AppPattern)next.Current);Sequence=next;}
    public void ReadNative(HardwarePattern p)
    {
        if(File.Exists(NativePath))File.Copy(NativePath,NativePath+".before-read-"+DateTime.UtcNow.Ticks);
        WorkspaceFiles.SaveHardware(NativePath,p);Native=new(p);
    }
    public void EditNative(EditProposal proposal){var next=Native!.Fork();next.Apply(proposal);WorkspaceFiles.SaveHardware(NativePath,(HardwarePattern)next.Current);Native=next;}
    public void NativeHistory(bool redo){var next=Native!.Fork();if(redo)next.Redo();else next.Undo();WorkspaceFiles.SaveHardware(NativePath,(HardwarePattern)next.Current);Native=next;}
}
