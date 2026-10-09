using Sloop.Core.Sampling;
using Sloop.Workstation;
using System.Text;

int checks = 0;
void Check(bool condition, string name) { if (!condition) throw new Exception(name); checks++; }
void Reject(Action action, string name) { try { action(); } catch (Exception e) when (e is IOException or InvalidDataException or FormatException or ArgumentException or OperationCanceledException) { checks++; return; } throw new Exception(name); }
var root = Path.Combine(Path.GetTempPath(), "sloop-library-tests-" + Guid.NewGuid().ToString("N"));
Directory.CreateDirectory(root);
try {
    var input = Path.Combine(root, "take.wav");
    using (var writer = new BinaryWriter(File.Create(input))) {
        writer.Write(Encoding.ASCII.GetBytes("RIFF")); writer.Write(2036u); writer.Write(Encoding.ASCII.GetBytes("WAVEfmt "));
        writer.Write(16u); writer.Write((ushort)1); writer.Write((ushort)1); writer.Write(8000); writer.Write(16000); writer.Write((ushort)2); writer.Write((ushort)16);
        writer.Write(Encoding.ASCII.GetBytes("data")); writer.Write(2000u); for (int i=0;i<1000;i++) writer.Write((short)i);
    }
    var doc = new SampleDocument(PcmWave.Open(input)); doc.Trim(10, 990); doc.Split(500); doc.Save(input + ".edits");
    File.WriteAllText(input + ".chopaudio", ChopAudioSettings.Serialize([new(10,500,2,3,62)]));
    File.WriteAllLines(input + ".kitsettings", ["SLOOP-KIT-1", Convert.ToBase64String(Encoding.UTF8.GetBytes("Saved kit")), "Instrument", "60", "AverageChannels", "1.5", "False"]);
    var storeRoot = Path.Combine(root, "sessions"); var store = new SessionStore(storeRoot);
    SessionInput[] inputs = new[] { "", ".edits", ".kitsettings", ".chopaudio" }.Select(s => new SessionInput("samples/take.wav" + s, input + s)).ToArray();
    var first = store.Save("First session", inputs); var second = store.Save("Second session", inputs);
    Directory.CreateDirectory(Path.Combine(storeRoot,"stage-ignored")); File.WriteAllText(Path.Combine(storeRoot,"stage-ignored","manifest.json"),"garbage");
    var library = new SampleLibrary(storeRoot); var index = library.Index();
    Check(index.Samples.Length == 2 && index.Rejected == 0, "Index immutable snapshots, ignore staging");
    var sample = index.Samples.Single(s=>s.SessionId==first.Manifest.Id);
    Check(sample.SessionName == "First session" && sample.Chops == 2 && sample.Seconds == .125, "Source session and exact WAV metadata");
    var destination = Path.Combine(root,"active"); var name = library.Copy(sample,destination); var path = Path.Combine(destination,name);
    Check(File.ReadAllBytes(path).SequenceEqual(File.ReadAllBytes(input)), "Byte-exact WAV copy");
    foreach(var suffix in new[]{".edits",".kitsettings",".chopaudio"}) Check(File.ReadAllBytes(path+suffix).SequenceEqual(File.ReadAllBytes(input+suffix)), "Correct associated sidecar " + suffix);
    var restored=SampleLibrary.Validate(path);
    Check(restored.Start==10 && restored.End==990 && restored.Markers.SequenceEqual(new long[]{500}), "Exact source-frame edits");
    Check(ChopAudioSettings.Find(ChopAudioSettings.Parse(File.ReadAllText(path+".chopaudio")),10,500)?.RootNote==62, "Exact chop audio association");
    Check(SampleLibrary.ReadKit(path,2).Gain==1.5, "Saved kit settings");
    Check(library.Copy(index.Samples.Single(s=>s.SessionId==second.Manifest.Id),destination)==name && Directory.GetFiles(destination,"*.wav").Length==1, "Identical complete state deduplicates across sessions");
    Check(File.ReadAllBytes(Path.Combine(first.Directory,"samples/take.wav")).SequenceEqual(File.ReadAllBytes(input)), "Snapshot unchanged by reuse");
    File.WriteAllText(path+".chopaudio","[]");
    var fresh=library.Copy(sample,destination);
    Check(fresh!=name && File.ReadAllText(path+".chopaudio")=="[]", "Changed writable copy preserved; new copy restores snapshot state");
    using(var cancelled=new CancellationTokenSource()) {
        cancelled.Cancel(); Reject(()=>library.Index(cancelled.Token),"Cancelled indexing");
        var cancelledRoot=Path.Combine(root,"cancelled"); Reject(()=>library.Copy(sample,cancelledRoot,cancelled.Token),"Cancelled reuse");
        Check(!Directory.Exists(cancelledRoot),"Cancelled reuse publishes nothing");
    }
    using(var midCopy=new CancellationTokenSource()) {
        var cancelledRoot=Path.Combine(root,"mid-copy");
        Reject(()=>library.Copy(sample,cancelledRoot,midCopy.Token,new ImmediateProgress(_=>midCopy.Cancel())),"Cancellation during staged bytes");
        Check(!Directory.EnumerateFileSystemEntries(cancelledRoot).Any(),"Mid-copy cancellation cleans staging and publishes nothing");
    }
    // A damaged unrelated sample must not suppress a good sibling in the same snapshot.
    var mixed=store.Save("Mixed",inputs.Concat([new SessionInput("samples/bad.wav",input)]));
    File.WriteAllText(Path.Combine(mixed.Directory,"samples/bad.wav"),"broken");
    index=library.Index(); Check(index.Samples.Count(s=>s.SessionId==mixed.Manifest.Id)==1 && index.Rejected==1,"Per-asset corruption isolation");
    File.WriteAllText(Path.Combine(first.Directory,"samples/take.wav.chopaudio"),"[]");
    Reject(()=>library.Copy(sample,Path.Combine(root,"stale")),"Stale sidecar rejected before publication");
    Check(!Directory.Exists(Path.Combine(root,"stale")) || !Directory.EnumerateFiles(Path.Combine(root,"stale"),"*.wav").Any(),"Stale reuse publishes no WAV");
    Directory.Delete(second.Directory,true); Reject(()=>library.Copy(index.Samples.Single(s=>s.SessionId==second.Manifest.Id),destination),"Missing source session rejected even for duplicate");
    // Hash-valid sidecars can still be semantically invalid.
    File.WriteAllText(input+".edits","SLOOP-SAMPLE-1\n10\n990\n700\n500\n");
    var unordered=store.Save("Unordered",inputs); Check(!library.Index().Samples.Any(s=>s.SessionId==unordered.Manifest.Id),"Unordered exact chop metadata rejected");
    File.WriteAllText(input+".edits","SLOOP-SAMPLE-1\n10\n990\n500\n");
    File.WriteAllText(input+".chopaudio",ChopAudioSettings.Serialize([new(10,1001)]));
    var invalidAudio=store.Save("Bad audio",inputs); Check(!library.Index().Samples.Any(s=>s.SessionId==invalidAudio.Manifest.Id),"Out-of-source audio metadata rejected");
    File.WriteAllText(input+".chopaudio","[]");
    var unlisted=store.Save("Unlisted",[new("samples/take.wav",input)]);
    File.WriteAllText(Path.Combine(unlisted.Directory,"samples/take.wav.chopaudio"),"[]");
    Check(!library.Index().Samples.Any(s=>s.SessionId==unlisted.Manifest.Id),"Unmanifested sidecars cannot cross archive boundary");
    File.AppendAllText(input,"trailing"); Reject(()=>SampleLibrary.Validate(input),"Exact RIFF length required");
    Check(!Directory.EnumerateDirectories(destination).Any(),"No staging leftovers after success");
    Console.WriteLine($"Sample library: {checks} checks passed.");
} finally { Directory.Delete(root,true); }

sealed class ImmediateProgress(Action<long> action) : IProgress<long> { public void Report(long value) => action(value); }
