using Sloop.Workstation;

int checks=0;
void Check(bool condition,string message){checks++;if(!condition)throw new Exception(message);}
void Reject(Action action,string message){bool rejected=false;try{action();}catch(Exception e)when(e is InvalidOperationException or InvalidDataException){rejected=true;}Check(rejected,message);}
var root=Path.Combine(Path.GetTempPath(),"sloop-generation-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
try
{
    Directory.CreateDirectory(Path.Combine(root,"workspaces"));File.WriteAllText(Path.Combine(root,"workspaces","sound-0.sloop"),"original");
    Directory.CreateDirectory(Path.Combine(root,"sessions"));File.WriteAllText(Path.Combine(root,"sessions","large-private-library"),"never copied");
    var store=new WorkspaceGenerationStore(root);Check(store.ActiveRoot==root,"Legacy migration starts with existing live workspace.");
    var first=store.StageCopy(root);Check(store.ActiveRoot==root,"Preparing does not activate candidate.");
    Check(File.ReadAllText(Path.Combine(first.Root,"workspaces","sound-0.sloop"))=="original","Staged copy preserves existing files.");
    Check(!Directory.Exists(Path.Combine(first.Root,"sessions")),"Private Library is not recursively included.");
    Reject(()=>store.Commit(first),"Unvalidated stages cannot commit.");
    File.WriteAllText(Path.Combine(first.Root,"workspaces","sound-0.sloop"),"candidate");
    Check(File.ReadAllText(Path.Combine(root,"workspaces","sound-0.sloop"))=="original","Editing stage leaves prior live file intact.");
    store.MarkPrepared(first);store.Commit(first);Check(new WorkspaceGenerationStore(root).ActiveRoot==first.Root,"Durable activation survives process restart.");
    File.WriteAllText(Path.Combine(first.Root,"workspaces","sound-0.sloop"),"live edit");
    var a=store.StageCopy(first.Root);var b=store.StageCopy(first.Root);store.MarkPrepared(a);store.MarkPrepared(b);store.Commit(a);
    Reject(()=>new WorkspaceGenerationStore(root).Commit(b),"Stale prepared scene/session cannot overwrite newer generation.");
    Check(File.ReadAllText(Path.Combine(first.Root,"workspaces","sound-0.sloop"))=="live edit","Old generation retains previous live edits.");
    store.RestorePrevious();Check(store.ActiveRoot==first.Root,"Rollback restores prior generation.");
    var next=store.StageCopy(first.Root);store.MarkPrepared(next);store.Commit(next);
    File.WriteAllText(Path.Combine(root,"workspace-current"),"../../outside");Check(new WorkspaceGenerationStore(root).ActiveRoot==first.Root,"Corrupt current pointer recovers previous valid generation.");
    Check(File.ReadAllText(Path.Combine(root,"workspace-current"))==Path.GetFileName(first.Root),"Recovery repairs durable current pointer.");
    Reject(()=>store.MarkPrepared(new(root,root)),"External directory cannot masquerade as prepared generation.");
    File.WriteAllText(Path.Combine(root,"workspace-current"),Guid.NewGuid().ToString("N"));Check(store.ActiveRoot==first.Root,"Missing generation recovers previous state.");
    var empty=Path.Combine(root,"empty-source");Directory.CreateDirectory(empty);var noAssets=store.StageCopy(empty);store.MarkPrepared(noAssets);store.Commit(noAssets);Check(store.ActiveRoot==noAssets.Root,"Empty session clears prior assets instead of retaining accidental files.");
    Check(Directory.Exists(first.Root),"Previous generation is preserved for recovery.");
    var recovered=store.RecoverValidRoot(path=>{if(!File.Exists(Path.Combine(path,"workspaces","sound-0.sloop")))throw new InvalidDataException("Missing editor document.");});
    Check(recovered==first.Root,"Whole workspace validation recovers previous generation instead of mixing default documents.");
    var corrupt=store.StageCopy(first.Root);store.MarkPrepared(corrupt);store.Commit(corrupt);
    var fallback=store.RecoverValidRoot(_=>throw new KeyNotFoundException("Malformed settings"));
    Check(fallback==root,"Invalid current and previous content recover retained legacy workspace.");
    Check(Directory.Exists(corrupt.Root)&&Directory.Exists(first.Root),"Content recovery retains invalid generations for inspection.");
    Console.WriteLine($"Session integration: {checks} checks passed.");
}
finally{Directory.Delete(root,true);}
