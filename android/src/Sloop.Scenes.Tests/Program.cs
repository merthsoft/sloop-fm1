using System.Collections.Immutable;
using Sloop.Scenes;
using Sloop.Sequencing;
using Sloop.SoundDesign;

int passed = 0;
void Test(string name, Action action) { action(); passed++; Console.WriteLine($"PASS {name}"); }
void Check(bool value) { if (!value) throw new Exception("Assertion failed."); }
void Reject(Action action) { try { action(); } catch (ArgumentException) { return; } catch (InvalidOperationException) { return; } catch (NotSupportedException) { return; } throw new Exception("Expected rejection."); }
var pattern = new AppPattern(Guid.NewGuid(), Guid.NewGuid(), 960, new(3840), []);
var envelope = new Envelope(99,99,99,99,99,99,99,0);
var sound = new SoundState(new Patch(Enumerable.Range(0,6).Select(_ => new Operator(envelope)).ToImmutableArray(),
    envelope, 1,0,true,0,0,0,0,true,LfoWave.Triangle,0,24,"SCENE"), MacroContext.Neutral);
var scene = new SceneSnapshot(Guid.NewGuid(), Guid.NewGuid(), "Verse", 120, [new("synth:0", sound)], pattern, []);
var second = scene with { Id = Guid.NewGuid(), Revision = Guid.NewGuid(), Name = "Chorus" };
var scenes = new[] { scene, second }.ToDictionary(s => s.Id);
var epoch = Guid.NewGuid();
MusicalPosition Position(long tick) => new(epoch, new(tick), 960,4,new(3840));
Test("typed snapshot validation", () => { scene.Validate(); Reject(() => (scene with { Sounds = [new("x",sound), new("x",sound)] }).Validate()); });
Test("snapshot isolation", () => { var edited = pattern with { Revision = Guid.NewGuid(), Length = new(7680) }; Check(scene.AppPattern!.Length.Value == 3840 && edited.Length.Value == 7680); });
var arrangement = new Arrangement(Guid.NewGuid(), "Song", [new(scene.Id,2), new(second.Id,1)], false);
Test("ordered repeats and completion", () => { var c = new ArrangementCursor(arrangement, scenes); Check(c.Current == scene.Id); Check(c.Advance() == scene.Id); Check(c.Advance() == second.Id); Check(c.Advance() is null && c.Complete); Check(c.Advance() is null); });
Test("loop arrangement", () => { var c = new ArrangementCursor(arrangement with { Loop = true }, scenes); c.Advance(); c.Advance(); Check(c.Advance() == scene.Id); });
Test("missing scene rejects", () => Reject(() => (arrangement with { Steps = [new(Guid.NewGuid(),1)] }).Validate(scenes)));
var lane = new AutomationLane(Guid.NewGuid(), new("midi:0", AutomationParameter.MidiControlChange, 74,0,127), AutomationShape.Linear, [new(new(100),0),new(new(200),100)]);
Test("automation hold interpolation and pre-roll", () => { Check(lane.Evaluate(new(0),pattern.Length) is null); Check(lane.Evaluate(new(150),pattern.Length) == 50); Check(lane.Evaluate(new(300),pattern.Length) == 100); Check((lane with { Shape = AutomationShape.Hold }).Evaluate(new(150),pattern.Length) == 0); });
Test("automation duplicate and range rejection", () => { Reject(() => (lane with { Points = [new(new(100),0),new(new(100),1)] }).Validate(pattern.Length)); Reject(() => (lane with { Points = [new(new(0),128)] }).Validate(pattern.Length)); });
Test("prepare queue exact commit reconcile", () => { var s = new SceneScheduler(Guid.NewGuid()); var p = s.Prepare(scene,s.LiveRevision,SceneRestoreMode.AppOnly); Check(s.Queue(p.Token,Position(960),SceneBoundary.Beat).Due.Value == 1920); Check(s.Commit(Position(1919)) is null); var commit = s.Commit(Position(1920))!; Check(s.Commit(Position(1920)) is null); s.Reconcile(new(commit.Token,RestoreOutcome.Applied,"app swap")); Check(s.LiveRevision == commit.AfterRevision && s.AwaitingReconciliation is null); });
Test("cancel and stale parent", () => { var s = new SceneScheduler(Guid.NewGuid()); Reject(() => s.Prepare(scene,Guid.NewGuid(),SceneRestoreMode.AppOnly)); var p = s.Prepare(scene,s.LiveRevision,SceneRestoreMode.AppOnly); s.Queue(p.Token,Position(0),SceneBoundary.Bar); Check(s.Cancel(p.Token)); Check(s.Commit(Position(3840)) is null); Reject(() => s.Queue(p.Token,Position(0),SceneBoundary.Bar)); });
Test("late boundary rejects", () => { var s = new SceneScheduler(Guid.NewGuid()); var p = s.Prepare(scene,s.LiveRevision,SceneRestoreMode.AppOnly); s.Queue(p.Token,Position(0),SceneBoundary.Phrase); Reject(() => s.Commit(Position(3841))); Check(s.Pending is null); });
Test("transport epoch invalidates", () => { var s = new SceneScheduler(Guid.NewGuid()); var p = s.Prepare(scene,s.LiveRevision,SceneRestoreMode.AppOnly); s.Queue(p.Token,Position(0),SceneBoundary.Beat); Reject(() => s.Commit(Position(960) with { TransportEpoch = Guid.NewGuid() })); });
Test("atomic hardware rejected app preparation usable", () => { var s = new SceneScheduler(Guid.NewGuid()); Reject(() => s.Prepare(scene,s.LiveRevision,SceneRestoreMode.HardwareAtomic)); s.Prepare(scene,s.LiveRevision,SceneRestoreMode.AppOnly); });
Test("unknown reconciliation blocks then partial resolves", () => { var s = new SceneScheduler(Guid.NewGuid()); var p = s.Prepare(scene,s.LiveRevision,SceneRestoreMode.HardwareReconciled); s.Queue(p.Token,Position(0),SceneBoundary.Beat); s.Commit(Position(960)); s.Reconcile(new(p.Token,RestoreOutcome.Unknown,"disconnected")); Reject(() => s.Prepare(scene,s.LiveRevision,SceneRestoreMode.AppOnly)); s.Reconcile(new(p.Token,RestoreOutcome.Partial,"readback differs")); Check(s.AwaitingReconciliation is null && s.LastReconciliation!.Outcome == RestoreOutcome.Partial); });
var catalog = new SceneCatalog(1,[scene,second],[arrangement],[new(scene.Id,lane)]);
Test("persistence preserves typed sound pattern identities and automation", () => { var copy = SceneStore.Decode(SceneStore.Encode(catalog)); Check(copy.Scenes[0].AppPattern!.Id == pattern.Id); Check(PatchCodec.ContentEquals(copy.Scenes[0].Sounds[0].State.Patch,sound.Patch)); Check(copy.Automation[0].Lane.Evaluate(new(150),pattern.Length) == 50); Check(copy.Arrangements[0].Steps[1].SceneId == second.Id); });
Test("save replace load", () => { var path = Path.Combine(AppContext.BaseDirectory,$"sloop-scenes-{Guid.NewGuid():N}.json"); try { SceneStore.Save(path,catalog); SceneStore.Save(path,catalog with { Arrangements = [] }); Check(SceneStore.Load(path).Arrangements.IsEmpty); } finally { File.Delete(path); } });
Test("schema and orphan automation reject", () => { Reject(() => SceneStore.Encode(catalog with { Version = 2 })); Reject(() => SceneStore.Encode(catalog with { Automation = [new(Guid.NewGuid(),lane)] })); });
Test("capture replace and stale edit", () => { var captured = SceneEditing.Capture("Captured",120,scene.Sounds,pattern); Check(captured.Id != scene.Id); var replaced = SceneEditing.Replace(captured,captured.Revision,captured with { Name = "Renamed" }); Check(replaced.Id == captured.Id && replaced.Revision != captured.Revision); Reject(() => SceneEditing.Replace(replaced,captured.Revision,replaced)); });
Test("arrangement reorder preserves source", () => { var moved = SceneEditing.Move(arrangement,0,1,scenes); Check(moved.Steps[0].SceneId == second.Id && arrangement.Steps[0].SceneId == scene.Id); });
Test("native identity roundtrip and app-only rejection", () => { var native = scene with { NativePatterns = [new(Guid.NewGuid(),Guid.NewGuid(),"fm1:serial",0,2)] }; var copy = SceneStore.Decode(SceneStore.Encode(catalog with { Scenes = [native,second] })); Check(copy.Scenes[0].NativePatterns[0] == native.NativePatterns[0]); var s = new SceneScheduler(Guid.NewGuid()); Reject(() => s.Prepare(native,s.LiveRevision,SceneRestoreMode.AppOnly)); s.Prepare(native,s.LiveRevision,SceneRestoreMode.HardwareReconciled); });
Test("macro physical ranges reject", () => Reject(() => (lane with { Target = new("synth:0",AutomationParameter.SoundMacro,1,-100,100) }).Validate(pattern.Length)));
Test("catalog rename keeps arrangement references",()=> {
    var renamed=CatalogEditing.Replace(catalog,scene.Id,scene.Revision,scene with{Name="New verse"});
    Check(renamed.Scenes[0].Name=="New verse" && renamed.Arrangements[0].Steps[0].SceneId==scene.Id);
    Reject(()=>CatalogEditing.Replace(renamed,scene.Id,scene.Revision,scene));
});
Test("replacement rejects incompatible automation without modifying source",()=> {
    Reject(()=>CatalogEditing.Replace(catalog,scene.Id,scene.Revision,scene with{AppPattern=pattern with{Length=new(150)}}));
    Check(catalog.Scenes[0].AppPattern!.Length==pattern.Length);
});
Test("duplicate copies independent automation identities",()=> {
    var copy=CatalogEditing.Duplicate(catalog,scene.Id,"Verse copy");var added=copy.Scenes[^1];
    Check(added.Id!=scene.Id && added.Revision!=scene.Revision && copy.Arrangements.Length==catalog.Arrangements.Length);
    var copiedLane=copy.Automation.Single(a=>a.SceneId==added.Id).Lane;
    Check(copiedLane.Id!=lane.Id && copiedLane.Points.SequenceEqual(lane.Points));
});
Test("referenced scene deletion is blocked",()=>Reject(()=>CatalogEditing.Delete(catalog,scene.Id,scene.Revision)));
Test("scene deletion removes only its automation",()=> {
    var independent=catalog with{Arrangements=[]};var deleted=CatalogEditing.Delete(independent,scene.Id,scene.Revision);
    Check(deleted.Scenes.Length==1 && deleted.Scenes[0].Id==second.Id && deleted.Automation.IsEmpty);
    Reject(()=>CatalogEditing.Delete(independent,scene.Id,Guid.NewGuid()));
});
Test("invalid duplicate name leaves source intact",()=> {
    Reject(()=>CatalogEditing.Duplicate(catalog,scene.Id," "));Check(catalog.Scenes.Length==2);
});
Console.WriteLine($"{passed} scene scenarios passed.");


