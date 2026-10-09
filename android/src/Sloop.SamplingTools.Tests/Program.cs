using Sloop.Core.Sampling;

int checks = 0;
void Check(bool value, string message) { checks++; if (!value) throw new Exception(message); }
void Reject(Action action) { bool rejected = false; try { action(); } catch (ArgumentException) { rejected = true; } Check(rejected, "Expected invalid edit rejection"); }
string folder = Path.Combine(AppContext.BaseDirectory, "sloop-sampling-" + Guid.NewGuid().ToString("N"));
Directory.CreateDirectory(folder);
try
{
    string path = Path.Combine(folder,"source.wav");
    const int rate = 8000, frames = 8000;
    using (var writer = new BinaryWriter(File.Create(path))) {
        writer.Write("RIFF"u8); writer.Write(36 + frames*4); writer.Write("WAVEfmt "u8); writer.Write(16);
        writer.Write((ushort)1); writer.Write((ushort)2); writer.Write(rate); writer.Write(rate*4); writer.Write((ushort)4); writer.Write((ushort)16); writer.Write("data"u8); writer.Write(frames*4);
        for (int i = 0; i < frames; i++) { short value = (short)(i is >= 1600 and < 1616 or >= 4000 and < 4016 or >= 6400 and < 6416 ? 30000 : 0); writer.Write(value); writer.Write((short)-value); }
    }
    var original = File.ReadAllBytes(path); var source = PcmWave.Open(path); var doc = new SampleDocument(source);
    var tapDoc = new SampleDocument(source); tapDoc.Trim(1000, 7000); tapDoc.Split(4000);
    var taps = new TapChopReview(tapDoc);
    Check(!taps.AddPlaybackPosition(1000,7000,0), "Tap ignores trim start");
    Check(taps.AddPlaybackPosition(1000,7000,200,50) && taps.Boundaries.SequenceEqual(new long[]{2200}), "Tap uses playback milliseconds, source offset and latency");
    Check(!taps.AddPlaybackPosition(1000,7000,200,50), "Duplicate tap ignored");
    Check(!taps.AddPlaybackPosition(1000,7000,900), "Tap outside preview ignored");
    Check(taps.AddPlaybackPosition(1000,7000,400) && taps.Undo() && taps.Boundaries.Count == 1, "Last tap undo before apply");
    Check(tapDoc.Markers.SequenceEqual(new long[]{4000}), "Tap draft preserves saved chops");
    taps.Apply(tapDoc); Check(tapDoc.Markers.SequenceEqual(new long[]{2200}), "Tap apply replaces boundaries");
    Check(tapDoc.Undo() && tapDoc.Markers.SequenceEqual(new long[]{4000}) && !taps.IsCurrent(tapDoc), "Single undo restores chops and invalidates draft");
    bool staleTapRejected = false; try { taps.Apply(tapDoc); } catch (InvalidOperationException) { staleTapRejected = true; }
    Check(staleTapRejected, "Stale tap review cannot apply");
    var limitedTaps = new TapChopReview(new SampleDocument(source));
    for (int i=1;i<=15;i++) Check(limitedTaps.AddPlaybackPosition(0,frames,i*20), "Tap accepted within capacity");
    Check(!limitedTaps.AddPlaybackPosition(0,frames,400), "Tap capacity limited to 15 boundaries");
    Check(limitedTaps.Undo() && limitedTaps.AddPlaybackPosition(0,frames,400), "Undo frees capacity for a replacement tap");
    var offsetTaps = new TapChopReview(new SampleDocument(source));
    Check(!offsetTaps.AddPlaybackPosition(2000,6000,0) && !offsetTaps.AddPlaybackPosition(2000,6000,20,50), "Partial preview start and latency-clamped taps do not invent boundaries");
    Check(!offsetTaps.AddPlaybackPosition(2000,6000,500), "Partial preview end is excluded");
    Check(!offsetTaps.AddPlaybackPosition(-1,6000,100) && !offsetTaps.AddPlaybackPosition(2000,frames+1,100)
        && !offsetTaps.AddPlaybackPosition(6000,2000,100), "Invalid preview ranges rejected without markers");
    Check(!offsetTaps.AddPlaybackPosition(2000,6000,-1) && !offsetTaps.AddPlaybackPosition(2000,6000,100,-1)
        && !offsetTaps.AddPlaybackPosition(2000,6000,100,1001), "Invalid position and latency rejected");
    Check(!offsetTaps.AddPlaybackPosition(2000,6000,int.MaxValue), "Large playback position cannot wrap into a valid chop");
    Check(offsetTaps.AddPlaybackPosition(2000,6000,300) && offsetTaps.AddPlaybackPosition(2000,6000,100)
        && offsetTaps.Boundaries.SequenceEqual(new long[]{2800,4400}), "Scrubbing backward produces sorted tap boundaries");
    Check(offsetTaps.Undo() && offsetTaps.Boundaries.SequenceEqual(new long[]{4400}), "Tap undo removes last gesture rather than largest sorted boundary");
    var unchangedDoc = new SampleDocument(source);
    var identityTaps = new TapChopReview(unchangedDoc);
    Check(!identityTaps.IsCurrent(new SampleDocument(source)), "A replacement document with the same source and revision cannot adopt an old draft");
    var replacementDoc = new SampleDocument(source);
    bool foreignTapRejected = false;
    try { identityTaps.Apply(replacementDoc); } catch (InvalidOperationException) { foreignTapRejected = true; }
    Check(foreignTapRejected && !replacementDoc.CanUndo && replacementDoc.Markers.Count == 0, "Foreign tap apply is rejected without creating history or edits");
    unchangedDoc.Trim(0,frames);
    Check(identityTaps.IsCurrent(unchangedDoc), "No-op trim keeps draft usable");
    unchangedDoc.Trim(100,frames);
    Check(!identityTaps.AddPlaybackPosition(100,frames,100) && identityTaps.Boundaries.Count == 0, "An edited document rejects additional taps without corrupting the draft");
    Check(doc.Split(2000) && doc.Split(5000),"Split"); long revision = doc.Revision;
    Check(!doc.MoveBoundary(2000,5000) && doc.Revision == revision,"Invalid boundary leaves history unchanged");
    Check(doc.MoveBoundary(2000,2101),"Move exact frame"); Check(doc.Markers.SequenceEqual(new long[]{2101,5000}),"Ordered move");
    Check(doc.Undo() && doc.Markers[0] == 2000,"Move undo");
    doc.SetSliceRange(1,1901,5101); Check(doc.Slices()[1] == (1901,5101),"Atomic slice edges");
    Check(doc.Undo() && doc.Slices()[1] == (2000,5000),"Single undo restores both edges");
    Reject(() => doc.SetSliceRange(1,0,5000)); Reject(() => doc.SetSliceRange(1,4000,4000));
    Check(doc.RemoveBoundary(2000) && doc.Slices().Length == 2,"Merge"); Check(doc.Undo() && doc.Slices().Length == 3,"Merge undo");
    Reject(() => doc.ReplaceBoundaries([3000,2000])); Reject(() => doc.ReplaceBoundaries([2000,2000]));
    var proposal = SamplingTools.Detect(source,0,frames,doc.Revision,.8,80);
    Check(proposal.Boundaries.SequenceEqual(new long[]{1600,4000,6400}),"Detect opposite stereo transients without downmix cancellation");
    Check(SamplingTools.Detect(source,0,1000,0).Boundaries.Count == 0,"Silence doesn't invent chops");
    var trimmedProposal = SamplingTools.Detect(source,2000,6000,0,.8,80);
    Check(trimmedProposal.Boundaries.SequenceEqual(new long[]{4000}),"Proposals stay within trim and retain edge spacing");
    Reject(() => SamplingTools.Detect(source,0,frames,0,double.NaN));
    Reject(() => SamplingTools.Peaks(source,100,100));
    Check(doc.Markers.SequenceEqual(new long[]{2000,5000}),"Proposal doesn't edit");
    doc.ReplaceBoundaries(proposal.Boundaries); Check(doc.Slices().Length == 4,"Explicit proposal apply"); Check(doc.Undo() && doc.Markers.SequenceEqual(new long[]{2000,5000}),"Proposal undo coherent");
    var cancelled = new CancellationToken(true); bool didCancel = false;
    try { SamplingTools.Detect(source,0,frames,0,token:cancelled); } catch(OperationCanceledException) { didCancel = true; } Check(didCancel,"Cancellation");
    var peaks = SamplingTools.Peaks(source,1500,1700,200); Check(peaks[100] > .9 && peaks[0] == 0,"Zoom envelope uses visible source frames");
    var viewport = new SampleViewport(frames); viewport.Zoom(4,4000); Check(viewport.Start == 3000 && viewport.End == 5000,"Anchor zoom");
    Check(viewport.FrameAt(0) == 3000 && viewport.FrameAt(1) == 4999,"Half open cursor mapping"); viewport.Pan(-99999); Check(viewport.Start == 0 && viewport.Length == 2000,"Pan clamps"); viewport.Zoom(.01,0); Check(viewport.Start == 0 && viewport.End == frames,"Zoom out clamps");
    viewport.Show(4000,4001); Check(viewport.FrameAt(.5) == 4000,"Single frame zoom"); Reject(() => viewport.Show(5,5));
    doc.Trim(100,7900); string edits = path + ".edits"; doc.Save(edits); var restored = SampleDocument.Restore(source,edits);
    Check(restored.Start == 100 && restored.End == 7900 && restored.Markers.SequenceEqual(doc.Markers) && !restored.CanUndo,"Persistence");
    source.Export(Path.Combine(folder,"slice.wav"),1901,5101); Check(PcmWave.Open(Path.Combine(folder,"slice.wav")).Frames == 3200,"Exact extraction");
    Reject(() => source.Export(path,0,frames));
    Check(File.ReadAllBytes(path).SequenceEqual(original),"Original immutable through edits, analysis and export");
    Console.WriteLine($"Sampling tools: {checks} checks passed.");
}
finally { Directory.Delete(folder,true); }

