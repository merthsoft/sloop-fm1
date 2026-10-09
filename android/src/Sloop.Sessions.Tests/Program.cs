using Sloop.Workstation;
using System.IO.Compression;
using Sloop.SoundDesign;
using Sloop.Sequencing;

var root=Path.Combine(Directory.GetCurrentDirectory(),".session-test-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
int passed=0;
void Check(bool ok,string description){if(!ok)throw new Exception(description);passed++;}
void Reject(Action action,string description){try{action();}catch(Exception e)when(e is InvalidDataException or IOException or ArgumentException){passed++;return;}throw new Exception("Accepted "+description);}
byte[] Archive(params (string Path,byte[] Bytes)[] entries){using var bytes=new MemoryStream();using(var zip=new ZipArchive(bytes,ZipArchiveMode.Create,true))foreach(var e in entries){using var stream=zip.CreateEntry(e.Path).Open();stream.Write(e.Bytes);}return bytes.ToArray();}
try{
    var store=new SessionStore(Path.Combine(root,"library"));var original=Path.Combine(root,"original.wav");File.WriteAllBytes(original,[1,2,3,4]);var edits=Path.Combine(root,"edits");File.WriteAllText(edits,"immutable edit settings");
    var saved=store.Save("First session",[new("samples/original.wav",original),new("samples/original.wav.edits",edits)],presets:[Guid.NewGuid()]);store.Activate(saved.Manifest.Id);
    Check(store.Current!.Manifest.Name=="First session","active session");
    using var exported=new MemoryStream();store.Export(saved.Manifest.Id,exported);exported.Position=0;var imported=store.Import(exported);
    Check(imported.Manifest.Id!=saved.Manifest.Id,"collision creates new ID");Check(File.ReadAllBytes(Path.Combine(imported.Directory,"samples/original.wav")).SequenceEqual(new byte[]{1,2,3,4}),"original round trip");Check(imported.Manifest.PresetIds.SequenceEqual(saved.Manifest.PresetIds),"references round trip");
    Check(store.Current!.Manifest.Id==saved.Manifest.Id,"import preserves current");
    foreach(var path in new[]{"../escape","/absolute","C:/escape","a\\b","a/../b","a//b","CON.txt","a. ","manifest.json/../escape"})Reject(()=>store.Import(new MemoryStream(Archive((path,new byte[]{1})))),path);
    Reject(()=>store.Import(new MemoryStream(Archive(("x",new byte[]{1}),("X",new byte[]{2})))),"case-insensitive duplicate");
    Reject(()=>store.Import(new MemoryStream(Archive(Enumerable.Range(0,514).Select(i=>("x"+i,new byte[]{1})).ToArray()))),"excess entries");
    Reject(()=>store.Save("broken",[new("samples/missing",Path.Combine(root,"missing"))]),"missing asset");
    Reject(()=>store.Save("duplicate",[new("a",original),new("A",original)]),"save duplicate");
    var manifest=File.ReadAllBytes(Path.Combine(saved.Directory,"manifest.json"));
    Reject(()=>store.Import(new MemoryStream(Archive(("manifest.json",manifest),("samples/original.wav",new byte[]{4,3,2,1}),("samples/original.wav.edits",File.ReadAllBytes(edits))))),"hash mismatch");
    Reject(()=>store.Import(new MemoryStream(Archive(("manifest.json",manifest),("samples/original.wav",File.ReadAllBytes(original)),("samples/original.wav.edits",File.ReadAllBytes(edits)),("extra",new byte[]{1})))),"unreferenced asset");
    var newer=System.Text.Json.JsonSerializer.SerializeToUtf8Bytes(saved.Manifest with{Schema=2});Reject(()=>store.Import(new MemoryStream(Archive(("manifest.json",newer)))),"newer schema");
    Reject(()=>store.Import(new MemoryStream(Archive(("manifest.json",new byte[1024*1024+1])))),"manifest size");
    using var second=new MemoryStream(exported.ToArray());Reject(()=>store.Import(second,_=>throw new InvalidDataException("invalid document")),"content validation failure");
    Check(store.List().Length==2,"failed imports do not publish");Check(!Directory.GetDirectories(Path.Combine(root,"library"),"stage-*").Any(),"failed staging cleaned");
    File.WriteAllText(Path.Combine(imported.Directory,"samples/original.wav.edits"),"tampered");Reject(()=>store.Activate(imported.Manifest.Id),"tampered asset");Check(store.Current!.Manifest.Id==saved.Manifest.Id,"failed activation preserves current");
    var next=store.Save("Second",[new("original",original)]);store.Activate(next.Manifest.Id);Check(File.ReadAllText(Path.Combine(root,"library/current.previous"))==saved.Manifest.Id.ToString("N"),"previous pointer retained");
    var preset=store.SavePreset("Arp",new(){["Style"]="ArpUp"});Check(store.Presets().Single().Settings["Style"]=="ArpUp","preset round trip");
    var soundPath=Path.Combine(root,"sound.sloop");WorkspaceFiles.SaveSound(soundPath,new("local/track:0",new(FactoryLibrary.Get(0),MacroContext.Neutral)));
    var patternPath=Path.Combine(root,"pattern.sloop");var pattern=new AppPattern(Guid.NewGuid(),Guid.NewGuid(),960,new(15360),[]);WorkspaceFiles.SavePattern(patternPath,pattern);
    void ValidateDomain(SessionCandidate c){WorkspaceFiles.LoadSound(Path.Combine(c.Directory,"sound.sloop"),"local/track:0");WorkspaceFiles.LoadPattern(Path.Combine(c.Directory,"pattern.sloop"));}
    var domain=store.Save("Documents",[new("sound.sloop",soundPath),new("pattern.sloop",patternPath)],validateContent:ValidateDomain);using var docs=new MemoryStream();store.Export(domain.Manifest.Id,docs);docs.Position=0;var restored=store.Import(docs,ValidateDomain);
    Check(WorkspaceFiles.LoadPattern(Path.Combine(restored.Directory,"pattern.sloop")).Id==pattern.Id,"pattern IDs preserved");
    Check(PatchCodec.ContentEquals(WorkspaceFiles.LoadSound(Path.Combine(restored.Directory,"sound.sloop"),"target").State.Patch,FactoryLibrary.Get(0)),"sound content preserved");
    Check(File.ReadAllBytes(original).SequenceEqual(new byte[]{1,2,3,4}),"source preserved");Console.WriteLine($"Passed {passed} session/archive/recovery checks.");
}finally{Directory.Delete(root,true);}
