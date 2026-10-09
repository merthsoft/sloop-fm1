using System.Collections.Immutable;
using Sloop.Sequencing;
using Sloop.Workstation;

int checks=0;
void Check(bool condition,string message){checks++;if(!condition)throw new Exception(message);}
void Reject(Action action,string message){checks++;try{action();}catch(EditException){return;}throw new Exception("Expected rejection: "+message);}
const string prompt="key=Db scale=minor bars=4 seed=42 parts=bass,chords,melody,drums density=steady";
var draft=OfflineComposition.Generate(prompt);
var repeat=OfflineComposition.Generate(prompt);
Check(draft.Pattern.Notes.SequenceEqual(repeat.Pattern.Notes)&&draft.Pattern.Id==repeat.Pattern.Id,"Deterministic complete notes and IDs");
var reordered=OfflineComposition.Generate("density=steady parts=drums,melody,chords,bass seed=42 bars=4 scale=minor key=C#");
Check(draft.Pattern.Notes.SequenceEqual(reordered.Pattern.Notes),"Equivalent spelling and field/part order");
foreach(var invalid in new[]{prompt+" warm",prompt+" tempo=39",prompt+" seed=8",prompt.Replace("seed=42","seed=-1"),prompt.Replace("seed=42","seed=2147483648"),prompt.Replace("bars=4","bars=17"),prompt.Replace("bars=4","bars=0"),prompt.Replace("key=Db","key=H"),prompt.Replace("scale=minor","scale=blues"),prompt.Replace("parts=bass,chords,melody,drums","parts=bass,bass"),prompt.Replace("density=steady","density=wild"),prompt.Replace("seed=42 ",""),prompt.Replace("parts=bass,chords,melody,drums","parts=bass,")})
    Reject(()=>OfflineComposition.Parse(invalid),invalid);
Reject(()=>OfflineComposition.Generate(prompt,3),"Insufficient resolution");
Reject(()=>OfflineComposition.Generate(prompt,959),"Fractional sixteenth");
Reject(()=>OfflineComposition.Generate(prompt,96004),"Excess resolution");
foreach(string scale in new[]{"major","minor","dorian","mixolydian","harmonic-minor","major-pentatonic","minor-pentatonic"})
foreach(string key in new[]{"C","Db","B"})
foreach(string density in new[]{"sparse","steady","busy"})
foreach(int bars in new[]{1,16}) {
    var p=OfflineComposition.Generate($"key={key} scale={scale} bars={bars} seed=2147483647 parts=bass,chords,melody,drums density={density}",4);
    PatternValidation.Validate(p.Pattern);
    Check(p.Pattern.Length.Value==bars*16 && p.Pattern.Notes.Length>0,"Bounds across keys, scales, densities, lengths");
    var tones=PerformanceHarmony.Scale(p.Intent.Scale).ToHashSet();
    Check(p.Pattern.Notes.Where(n=>n.Channel==2).All(n=>tones.Contains((n.Pitch-p.Intent.Key+120)%12)),"Melody stays in scale");
    Check(p.Pattern.Notes.Where(n=>n.Channel==9).All(n=>n.Pitch is 36 or 38 or 42),"Drum vocabulary");
}
var bass=draft.Pattern.Notes.First(n=>n.Channel==0);
var edited=OfflineComposition.EditNote(draft,bass.Id,bass with {Pitch=45,Velocity=110,Start=new(1),Duration=new(20)});
Check(edited.Pattern.Notes.Single(n=>n.Id==bass.Id).Pitch==45,"Draft editing");
Reject(()=>OfflineComposition.EditNote(draft,bass.Id,bass with {Pitch=128}),"Invalid edited pitch");
Reject(()=>OfflineComposition.EditNote(draft,bass.Id,bass with {Channel=8}),"Moving between parts");
var deleted=OfflineComposition.EditNote(edited,bass.Id,null);
Check(!deleted.Pattern.Notes.Any(n=>n.Id==bass.Id),"Draft delete");
var next=OfflineComposition.Regenerate(edited,[CompositionPart.Bass]);
Check(next.Intent.Seed==43,"Regeneration increments seed");
Check(OfflineComposition.Parse(next.Prompt).Seed==43,"Regenerated prompt records resolved seed");
Check(next.Pattern.Notes.Where(n=>n.Channel==0).SequenceEqual(edited.Pattern.Notes.Where(n=>n.Channel==0)),"Keep retains all edited bass notes");
Check(!next.Pattern.Notes.Where(n=>n.Channel==2).SequenceEqual(edited.Pattern.Notes.Where(n=>n.Channel==2)),"Regenerated melody changes");
var keepAll=OfflineComposition.Regenerate(edited,edited.Intent.Parts.ToImmutableHashSet());
Check(keepAll.Pattern.Notes.OrderBy(n=>n.Id).SequenceEqual(edited.Pattern.Notes.OrderBy(n=>n.Id)),"Keep all exact contents");
var onlyBass=OfflineComposition.Generate("key=C scale=major bars=1 seed=0 parts=bass density=sparse");
Reject(()=>OfflineComposition.Regenerate(onlyBass,[CompositionPart.Drums]),"Unknown keep part");
Reject(()=>OfflineComposition.Regenerate(OfflineComposition.Generate(prompt.Replace("seed=42","seed=2147483647")),[]),"Seed overflow");
using(var cts=new CancellationTokenSource()) {
    cts.Cancel();bool canceled=false;
    try{OfflineComposition.Generate(prompt,cancellationToken:cts.Token);}catch(OperationCanceledException){canceled=true;}
    Check(canceled,"Generation cancellation");
}
var oldBass=new AppNote(Guid.NewGuid(),Guid.NewGuid(),new(0),new(100),36,90,0);
var retained=new AppNote(Guid.NewGuid(),Guid.NewGuid(),new(100),new(100),60,100,7);
var source=new AppPattern(Guid.NewGuid(),Guid.NewGuid(),960,new(15360),[oldBass,retained]);
var history=new EditHistory(source);var apply=OfflineComposition.ProposeApply(source,draft);
Check(ReferenceEquals(history.Current,source),"Proposal has no effects");
history.Apply(apply);
var applied=(AppPattern)history.Current;
Check(applied.Notes.Contains(retained)&&!applied.Notes.Contains(oldBass),"Requested channels replaced; others exact");
Check(history.AcceptedHistory.Length==1 && apply.Provenance.Seed==42,"One apply with seed provenance");
history.Undo();Check(((AppPattern)history.Current).Notes.SequenceEqual(source.Notes)&&((AppPattern)history.Current).Length==source.Length&&!history.CanUndo,"One undo restores exact previous contents and length");
Reject(()=>history.Apply(apply),"Stale captured source after undo");
history.Redo();Check(((AppPattern)history.Current).Notes.SequenceEqual(applied.Notes),"Redo generated contents");
Reject(()=>OfflineComposition.ProposeApply(source,draft,new([], [oldBass.Id])),"Protected event");
Reject(()=>PromptEditor.Propose(source,AppSelection.All(source),"transpose up 1 semitones",new([], [oldBass.Id])),"Prompt edit shares protected event locks");
Reject(()=>OfflineComposition.ProposeApply(source,draft,new([oldBass.PartId], [])),"Protected existing part");
Reject(()=>OfflineComposition.ProposeApply(source,draft,new([bass.PartId], [])),"Protected new part");
Reject(()=>OfflineComposition.ProposeApply(source with {TicksPerQuarter=480},draft),"Timing mismatch");
var late=retained with {Start=new(5000)};
Reject(()=>OfflineComposition.ProposeApply(source with {Notes=[oldBass,late]},onlyBass),"Shortening rejects retained material outside loop");
var shortSource=source with {Length=new(3840)};
var expanded=OfflineComposition.ProposeApply(shortSource,draft);
var expandedHistory=new EditHistory(shortSource);expandedHistory.Apply(expanded);expandedHistory.Undo();
Check(((AppPattern)expandedHistory.Current).Length.Value==3840,"Undo restores original shorter loop length");
Reject(()=>OfflineComposition.ProposeApply(source,draft with {Pattern=draft.Pattern with {Length=new(20000)}}),"Draft length inconsistent with intent");
Check(PromptEditor.Propose(source,AppSelection.All(source),"transpose up 1 semitones").Changes.Length==2,"Existing recipes still available");
// Optional controls preserve legacy defaults, change musical content predictably, and stay bounded.
Check(draft.Intent.Tempo==100 && draft.Intent.Progression==CompositionProgression.Classic && draft.Intent.Rhythm==CompositionRhythm.Straight,"Legacy defaults");
foreach(var progression in new[]{"classic","pop","minor-turnaround"})
foreach(var rhythm in new[]{"straight","offbeat","swing"})
foreach(var resolution in new[]{4,960}) {
    var text=prompt+$" tempo=240 progression={progression} rhythm={rhythm}";
    var controlled=OfflineComposition.Generate(text,resolution);OfflineComposition.ValidateDraft(controlled);
    Check(controlled.Pattern.Notes.SequenceEqual(OfflineComposition.Generate(text,resolution).Pattern.Notes),"Controls deterministic and valid at timing bounds");
    Check(OfflineComposition.Regenerate(controlled,[]).Intent.Tempo==240,"Regeneration retains controls");
}
Check(draft.Pattern.Notes.SequenceEqual(OfflineComposition.Generate(prompt+" tempo=40").Pattern.Notes),"Tempo does not alter symbolic notes");
Check(!draft.Pattern.Notes.SequenceEqual(OfflineComposition.Generate(prompt+" progression=pop").Pattern.Notes),"Progression changes notes");
Check(!draft.Pattern.Notes.SequenceEqual(OfflineComposition.Generate(prompt+" rhythm=offbeat").Pattern.Notes),"Offbeat changes timing");
foreach(var suffix in new[]{"tempo=241","tempo=slow","rhythm=shuffle","progression=random","tempo=90 tempo=100"})Reject(()=>OfflineComposition.Parse(prompt+" "+suffix),"Invalid control");
var saved=new SavedCompositionDraft(Guid.NewGuid(),"Edited bass",edited,[CompositionPart.Bass]);
var catalog=new CompositionCatalog(1,[saved]);
var decoded=CompositionDraftStore.Decode(CompositionDraftStore.Encode(catalog));
Check(decoded.Drafts[0].Draft.Pattern.Notes.SequenceEqual(edited.Pattern.Notes) && decoded.Drafts[0].Draft.Prompt==edited.Prompt && decoded.Drafts[0].Kept.SequenceEqual(saved.Kept),"Persistence retains edited identities, prompt and keep choices");
void InvalidStore(Action action,string message){checks++;try{action();}catch(Exception e)when(e is InvalidDataException or EditException or System.Text.Json.JsonException){return;}throw new Exception("Expected store rejection: "+message);}
InvalidStore(()=>CompositionDraftStore.Encode(catalog with {Version=2}),"Unknown schema");
InvalidStore(()=>CompositionDraftStore.Encode(catalog with {Drafts=[saved,saved]}),"Duplicate identities");
InvalidStore(()=>CompositionDraftStore.Encode(catalog with {Drafts=[saved with {Name=""}]}),"Missing name");
InvalidStore(()=>CompositionDraftStore.Encode(catalog with {Drafts=[saved with {Kept=[(CompositionPart)99]}]}),"Invalid kept part");
InvalidStore(()=>CompositionDraftStore.Encode(catalog with {Drafts=[saved with {Draft=edited with {Intent=edited.Intent with {Tempo=200}}}]}),"Prompt-intent mismatch");
InvalidStore(()=>CompositionDraftStore.Decode(new byte[CompositionDraftStore.MaximumBytes+1]),"Oversized input");
InvalidStore(()=>CompositionDraftStore.Decode("{}"u8),"Malformed catalog");
var directory=Path.Combine(Path.GetTempPath(),"sloop-composition-"+Guid.NewGuid().ToString("N"));
var path=Path.Combine(directory,"drafts.json");
try {
    Check(CompositionDraftStore.Load(path).Drafts.IsEmpty,"Missing catalog is empty");
    CompositionDraftStore.Save(path,catalog);var before=File.ReadAllBytes(path);
    InvalidStore(()=>CompositionDraftStore.Save(path,catalog with {Version=2}),"Invalid save");
    Check(before.SequenceEqual(File.ReadAllBytes(path)),"Rejected save leaves exact previous file");
    Check(CompositionDraftStore.Load(path).Drafts[0].Draft.Pattern.Notes.SequenceEqual(edited.Pattern.Notes),"Disk round trip");
    CompositionDraftStore.Save(path,CompositionDraftStore.Empty);Check(CompositionDraftStore.Load(path).Drafts.IsEmpty,"Atomic replacement supports deletion");
}finally{if(File.Exists(path))File.Delete(path);if(Directory.Exists(directory))Directory.Delete(directory);}
// Malformed retained drafts must reject through the public validation contract, not null dereferences.
InvalidStore(()=>CompositionDraftStore.Encode(catalog with {Drafts=[null!]}),"Null catalog entry");
InvalidStore(()=>CompositionDraftStore.Encode(catalog with {Drafts=[saved with {Draft=null!}]}),"Null draft");
InvalidStore(()=>CompositionDraftStore.Encode(catalog with {Drafts=[saved with {Draft=edited with {Intent=null!}}]}),"Null intent");
InvalidStore(()=>CompositionDraftStore.Encode(catalog with {Drafts=[saved with {Draft=edited with {Pattern=null!}}]}),"Null pattern");
InvalidStore(()=>CompositionDraftStore.Encode(catalog with {Drafts=[saved with {Draft=edited with {Pattern=edited.Pattern with {Notes=[null!]}}}]}),"Null note");
InvalidStore(()=>CompositionDraftStore.Decode("{\"Version\":1,\"Drafts\":[null]}"u8),"Null entry from JSON");
InvalidStore(()=>CompositionDraftStore.Decode("null"u8),"Null root from JSON");
InvalidStore(()=>CompositionDraftStore.Encode(catalog with {Drafts=[saved with {Kept=[CompositionPart.Bass,CompositionPart.Bass]}]}),"Duplicate kept parts");
InvalidStore(()=>CompositionDraftStore.Encode(catalog with {Drafts=[saved with {Name=new string('x',81)}]}),"Oversized name");
InvalidStore(()=>CompositionDraftStore.Encode(catalog with {Drafts=Enumerable.Range(0,129).Select(_=>saved with {Id=Guid.NewGuid()}).ToImmutableArray()}),"Catalog capacity");
var failureDirectory=Path.Combine(AppContext.BaseDirectory,"sloop-draft-failure-"+Guid.NewGuid().ToString("N"));
Directory.CreateDirectory(failureDirectory);
try {
    var damagedPath=Path.Combine(failureDirectory,"damaged.json");
    byte[] damaged="{\"Version\":1,\"Drafts\":["u8.ToArray();File.WriteAllBytes(damagedPath,damaged);
    InvalidStore(()=>CompositionDraftStore.Load(damagedPath),"Truncated disk catalog");
    Check(File.ReadAllBytes(damagedPath).SequenceEqual(damaged),"Failed load preserves exact damaged file for recovery");
    var blockedPath=Path.Combine(failureDirectory,"blocked.json");Directory.CreateDirectory(blockedPath);
    var sentinel=Path.Combine(blockedPath,"existing");File.WriteAllText(sentinel,"retain");
    bool saveFailed=false;try{CompositionDraftStore.Save(blockedPath,catalog);}catch(Exception e)when(e is IOException or UnauthorizedAccessException){saveFailed=true;}
    Check(saveFailed && File.ReadAllText(sentinel)=="retain","Failed publication preserves existing destination");
    Check(!Directory.EnumerateFiles(failureDirectory,"*.tmp").Any(),"Failed publication removes temporary payload");
}finally{Directory.Delete(failureDirectory,true);}

// The production preview owner must not stop unrelated playback or lose a newer preview to a late completion.
Guid? previewEpoch=Guid.NewGuid();int stops=0;CompositionAudition audition=null!;
audition=new(()=>previewEpoch,()=>{stops++;audition.Stop();}); // transport callbacks can re-enter cleanup
try{await audition.RunAsync(()=>Task.FromException(new IOException("Transport already running")));throw new Exception("Failed start was swallowed");}catch(IOException){}
audition.Stop();Check(stops==0,"Rejected start never acquires existing transport");
var firstDone=new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
previewEpoch=Guid.NewGuid();var firstRun=audition.RunAsync(()=>firstDone.Task);
try{await audition.RunAsync(()=>Task.FromException(new IOException("Already playing")));}catch(IOException){}
audition.Stop();audition.Stop();Check(stops==1,"Owned preview stops once despite failed retry and reentrant callbacks");
var secondDone=new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
previewEpoch=Guid.NewGuid();var secondRun=audition.RunAsync(()=>secondDone.Task);
firstDone.SetResult();await firstRun;audition.Stop();
Check(stops==2,"Old preview completion does not clear newer ownership");
secondDone.SetResult();await secondRun;
var replacedDone=new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
previewEpoch=Guid.NewGuid();var replacedRun=audition.RunAsync(()=>replacedDone.Task);
previewEpoch=Guid.NewGuid();audition.Stop();Check(stops==2,"External transport replacement is not stopped by stale preview cleanup");
replacedDone.SetResult();await replacedRun;
var failedDone=new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
previewEpoch=Guid.NewGuid();var failedRun=audition.RunAsync(()=>failedDone.Task);failedDone.SetException(new IOException("Disconnected"));
try{await failedRun;throw new Exception("Playback error was swallowed");}catch(IOException){}
audition.Stop();Check(stops==2,"Playback failure releases preview ownership");
var completedDone=new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
previewEpoch=Guid.NewGuid();var completedRun=audition.RunAsync(()=>completedDone.Task);completedDone.SetResult();await completedRun;
audition.Stop();Check(stops==2,"Normal completion releases preview ownership");
await audition.RunAsync(()=>Task.CompletedTask);audition.Stop();Check(stops==2,"Synchronous completion acquires no unrelated transport");
// Exercise the production loop across the same delayed worker boundary used by Android.
var realLoop=new PatternLoop();var reservedEpoch=realLoop.TransportEpoch;
var workerGate=new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
var noteStarted=new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
var emittedMidi=new List<byte[]>();using var realStop=new CancellationTokenSource();
var realPattern=new AppPattern(Guid.NewGuid(),Guid.NewGuid(),96,new(384),
    [new(Guid.NewGuid(),Guid.NewGuid(),new(0),new(384),60,100,0)]);
int realStops=0;
var realAudition=new CompositionAudition(()=>realLoop.TransportEpoch,()=>{realStops++;realStop.Cancel();});
var realRun=realAudition.RunAsync(()=>Task.Run(async()=> {
    await workerGate.Task;
    try{await realLoop.RunAsync(realPattern,120,bytes=>{emittedMidi.Add(bytes);if((bytes[0]&0xf0)==0x90)noteStarted.TrySetResult();},realStop.Token,reservedEpoch);}
    catch(OperationCanceledException)when(realStop.IsCancellationRequested){}
}));
try {
    Check(reservedEpoch!=Guid.Empty,"Transport ownership is advertised before worker startup");
    workerGate.SetResult();await noteStarted.Task.WaitAsync(TimeSpan.FromSeconds(5));
    realAudition.Stop();
    await realRun.WaitAsync(TimeSpan.FromSeconds(5));
    Check(realStops==1 && emittedMidi.Count(b=>(b[0]&0xf0)==0x90)==1 && emittedMidi.Count(b=>(b[0]&0xf0)==0x80)==1,
        "Draft stop after delayed real worker startup releases every owned MIDI note");
}finally{workerGate.TrySetResult();realStop.Cancel();await realRun;}
// Exercise the portable wire format rather than only its object validator.
byte[] Wire(params (string Name,string Json)[] entries)
{
    using var output=new MemoryStream();
    using(var zip=new System.IO.Compression.ZipArchive(output,System.IO.Compression.ZipArchiveMode.Create,true))
        foreach(var entry in entries) { using var writer=new StreamWriter(zip.CreateEntry(entry.Name).Open());writer.Write(entry.Json); }
    return output.ToArray();
}
SavedCompositionDraft ReadArchive(byte[] bytes)=>CompositionDraftArchive.Import(new MemoryStream(bytes));
var archive=CompositionDraftArchive.Export(saved);
var imported=ReadArchive(archive);
using(var fragmented=new FragmentedDraftStream(archive))
    Check(CompositionDraftArchive.Import(fragmented).Draft.Pattern.Notes.SequenceEqual(saved.Draft.Pattern.Notes),"Nonseekable short-read import retains exact editable notes");
InvalidStore(()=> {using var fragmented=new FragmentedDraftStream(new byte[CompositionDraftArchive.MaximumBytes+1]);CompositionDraftArchive.Import(fragmented);},"Nonseekable compressed input cap");
Check(CompositionDraftStore.Encode(new(1,[imported])).SequenceEqual(CompositionDraftStore.Encode(catalog)),"Portable roundtrip retains exact edited notes, IDs, intent, prompt, kept parts and generator");
var importedProposal=OfflineComposition.ProposeApply(source,imported.Draft);
var changedHistory=new EditHistory(source);
changedHistory.Apply(AppEditor.PutNote(source,oldBass.Id,oldBass with {Velocity=91}));
var changedBefore=changedHistory.Current;
Reject(()=>changedHistory.Apply(importedProposal),"Imported draft rejects stale sequence at final apply");
Check(ReferenceEquals(changedHistory.Current,changedBefore),"Rejected imported apply preserves current sequence");
var archiveJson=System.Text.Json.JsonSerializer.Serialize(new CompositionArchiveDocument(1,saved));
foreach(var archivePath in new[]{"../draft.json","/draft.json","C:/draft.json","draft\\json","Draft.json","nested/draft.json"})
    InvalidStore(()=>ReadArchive(Wire((archivePath,archiveJson))),"Unsafe/unexpected archive archivePath "+archivePath);
InvalidStore(()=>ReadArchive(Wire(("draft.json",archiveJson),("draft.json",archiveJson))),"Duplicate ZIP entries");
InvalidStore(()=>ReadArchive(Wire(("draft.json",archiveJson),("extra", "x"))),"Unexpected ZIP entries");
InvalidStore(()=>ReadArchive(archive[..^12]),"Truncated archive directory");
var corruptCrc=(byte[])archive.Clone();
for(var i=0;i<corruptCrc.Length-20;i++)
    if(corruptCrc.AsSpan(i,4).SequenceEqual(new byte[]{0x50,0x4b,0x01,0x02})) {corruptCrc[i+16]^=1;break;}
InvalidStore(()=>ReadArchive(corruptCrc),"Corrupt ZIP entry checksum");
InvalidStore(()=>ReadArchive(new byte[CompositionDraftArchive.MaximumBytes+1]),"Compressed input cap");
InvalidStore(()=>ReadArchive(Wire(("draft.json",new string(' ',CompositionDraftArchive.MaximumBytes+1)))),"Actual expanded cap");
foreach(var json in new[]{
    archiveJson.Replace("\"Version\":1","\"Version\":2"),
    archiveJson.Replace("\"Version\":1","\"Version\":1,\"Version\":1"),
    archiveJson.Replace("\"Version\":1","\"Version\":1,\"Unexpected\":0"),
    archiveJson.Replace("\"Seed\":42","\"Seed\":-1"),
    archiveJson.Replace("\"Tempo\":100","\"Tempo\":241"),
    archiveJson.Replace("\"Generator\":\"offline-composition/1\"","\"Generator\":\"unknown/2\""),
    archiveJson.Replace(",\"Generator\":\"offline-composition/1\"",""),
    archiveJson.Replace("\"Key\":1,",""),
    archiveJson.Replace("\"Velocity\":", "\"UnexpectedNote\":0,\"Velocity\":"),
    archiveJson[..^1], "null", "{}"})
    InvalidStore(()=>ReadArchive(Wire(("draft.json",json))),"Invalid archive JSON");
var archiveDirectory=Path.Combine(AppContext.BaseDirectory,"sloop-archive-"+Guid.NewGuid().ToString("N"));
foreach(var field in new[]{"Pitch","Velocity","Channel","Start","Duration"}) {
    var tree=System.Text.Json.Nodes.JsonNode.Parse(archiveJson)!;
    var note=tree["Draft"]!["Draft"]!["Pattern"]!["Notes"]![0]!;
    if(field is "Start" or "Duration") note[field]!["Value"]=long.MaxValue;
    else note[field]=128;
    InvalidStore(()=>ReadArchive(Wire(("draft.json",tree.ToJsonString()))),"Invalid note numeric bound "+field);
}
var duplicateTree=System.Text.Json.Nodes.JsonNode.Parse(archiveJson)!;
var duplicateNotes=duplicateTree["Draft"]!["Draft"]!["Pattern"]!["Notes"]!;
duplicateNotes[1]!["Id"]=duplicateNotes[0]!["Id"]!.DeepClone();
InvalidStore(()=>ReadArchive(Wire(("draft.json",duplicateTree.ToJsonString()))),"Duplicate imported note identities");
Directory.CreateDirectory(archiveDirectory);
try {
    var archivePath=Path.Combine(archiveDirectory,"drafts.json");CompositionDraftStore.Save(archivePath,catalog);
    var before=File.ReadAllBytes(archivePath);
    InvalidStore(()=>CompositionDraftArchive.Publish(archivePath,saved with {Draft=saved.Draft with {Generator="unknown"}}),"Invalid publication");
    Check(File.ReadAllBytes(archivePath).SequenceEqual(before),"Failed import publication retains exact local catalog");
    var published=CompositionDraftArchive.Publish(archivePath,imported);
    var publishedAgain=CompositionDraftArchive.Publish(archivePath,imported);
    var after=CompositionDraftStore.Load(archivePath);
    Check(after.Drafts.Length==3 && published.Id!=imported.Id && publishedAgain.Id!=published.Id,"Repeated imports create fresh identities and retain existing drafts");
    Check(CompositionDraftStore.Encode(new(1,[after.Drafts[0]])).SequenceEqual(before),"Import leaves previous saved draft exact");
    Check(after.Drafts[1].Draft.Pattern.Notes.SequenceEqual(saved.Draft.Pattern.Notes),"Published imported notes retain exact IDs and part metadata");
    var full=new CompositionCatalog(1,Enumerable.Range(0,128).Select(_=>saved with {Id=Guid.NewGuid()}).ToImmutableArray());
    CompositionDraftStore.Save(archivePath,full);var fullBytes=File.ReadAllBytes(archivePath);
    InvalidStore(()=>CompositionDraftArchive.Publish(archivePath,imported),"Full catalog import");
    Check(File.ReadAllBytes(archivePath).SequenceEqual(fullBytes),"Capacity failure retains all local drafts");
} finally { Directory.Delete(archiveDirectory,true); }
Console.WriteLine($"Offline composition: {checks} focused checks passed.");

sealed class FragmentedDraftStream(byte[] bytes) : Stream
{
    readonly MemoryStream input=new(bytes,false);
    public override bool CanRead=>true;
    public override bool CanSeek=>false;
    public override bool CanWrite=>false;
    public override long Length=>throw new NotSupportedException();
    public override long Position {get=>throw new NotSupportedException();set=>throw new NotSupportedException();}
    public override int Read(byte[] buffer,int offset,int count)=>input.Read(buffer,offset,Math.Min(count,37));
    public override void Flush()=>throw new NotSupportedException();
    public override long Seek(long offset,SeekOrigin origin)=>throw new NotSupportedException();
    public override void SetLength(long value)=>throw new NotSupportedException();
    public override void Write(byte[] buffer,int offset,int count)=>throw new NotSupportedException();
    protected override void Dispose(bool disposing){if(disposing)input.Dispose();base.Dispose(disposing);}
}
