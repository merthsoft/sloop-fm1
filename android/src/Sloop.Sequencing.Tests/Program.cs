using System.Collections.Immutable;
using Sloop.Sequencing;
var tests = new (string Name,Action Test)[]
{
    ("offline app commands preserve selection and support reviewed atomic undo",()=> {
        var p=App(); var selection=Select(p.Notes[0]);
        var proposal=PromptEditor.Propose(p,selection,"  TRANSPOSE up 12 semitones  ");
        var after=(AppPattern)proposal.After;
        Check(after.Notes[0]==p.Notes[0] with { Pitch=72 }); Check(after.Notes[1]==p.Notes[1]);
        Check(proposal.Provenance.RecipeVersion==PromptEditor.RecipeVersion && ReferenceEquals(proposal.Before,p));
        var h=new EditHistory(p); h.Apply(proposal); h.Undo(); Check(((AppPattern)h.Current).Notes==p.Notes);
        h.Redo(); Check(((AppPattern)h.Current).Notes==after.Notes);
        Reject(()=>h.Apply(proposal));
        Check(((AppPattern)PromptEditor.Propose(p,selection,"quantize 120 ticks").After).Notes[0].Start.Value==0);
        Check(((AppPattern)PromptEditor.Propose(p,selection,"resize shorter 20 ticks").After).Notes[0].Duration.Value==220);
        Check(((AppPattern)PromptEditor.Propose(p,selection,"velocity down 20").After).Notes[0].Velocity==80);
        Check(((AppPattern)PromptEditor.Propose(p,selection,"move later 100 ticks").After).Notes[0].Start.Value==113);
    }),
    ("offline commands reject unsupported clauses units amounts selections and locks",()=> {
        var p=App(); var selection=Select(p.Notes[0]);
        foreach(var text in new[]{"", "transpose up 12 semitones but keep bass", "not transpose up 12 semitones",
            "transpose up 12 semitones and velocity down 10", "make it syncopated", "add a fill", "move later 1 steps",
            "quantize 0 ticks", "velocity down 127", "transpose up 999999999999 semitones", "simplify!", "level hard"})
            Reject(()=>PromptEditor.Propose(p,selection,text));
        Reject(()=>PromptEditor.Propose(p,new([]),"simplify"));
        Reject(()=>PromptEditor.Propose(p,selection,"transpose up 1 semitones",new([p.Notes[0].PartId],[])));
        using var cancelled=new CancellationTokenSource(); cancelled.Cancel();
        try { PromptEditor.Propose(p,selection,"simplify",cancellationToken:cancelled.Token); throw new Exception("Expected cancellation"); }
        catch(OperationCanceledException) { }
        Check(p.Notes[0].Pitch==60);
    }),
    ("offline native simplification preserves kick locks and repeats accepted content",()=> {
        var p=Hardware(); var t=p.Tracks[3]; var selection=HSelect(t,0,4) with { DrumLanes=[4] };
        var a=PromptEditor.Propose(p,selection,"simplify",seed:1);
        var b=PromptEditor.Propose(p,selection,"simplify",seed:1);
        for(int i=0;i<64;i++) {
            var x=(DrumStep)((HardwarePattern)a.After).Tracks[3].Steps[i];
            var y=(DrumStep)((HardwarePattern)b.After).Tracks[3].Steps[i];
            Check(x.Id==y.Id && x.Micro==y.Micro && x.Fill==y.Fill && x.Lanes.SequenceEqual(y.Lanes));
        }
        Check(((DrumStep)((HardwarePattern)a.After).Tracks[3].Steps[0]).Lanes[0]==((DrumStep)t.Steps[0]).Lanes[0]);
        Check(a.Provenance.Seed==1);
        var h=new EditHistory(p);h.Apply(a);h.Undo();Check(((HardwarePattern)h.Current).Tracks==p.Tracks);
        h.Redo();Check(((HardwarePattern)h.Current).Tracks==((HardwarePattern)a.After).Tracks);
        foreach(var text in new[]{"resize longer 1 ticks","move later 1 ticks","quantize 120 ticks","velocity up 1"})
            Reject(()=>PromptEditor.Propose(p,selection,text));
        Reject(()=>PromptEditor.Propose(p,selection with { DrumLanes=[] },"simplify"));
        Reject(()=>PromptEditor.Propose(p,selection,"simplify",new([],[((DrumStep)t.Steps[0]).Lanes[4].Id]),seed:1));
        var level=PromptEditor.Propose(p,selection,"level ghost");
        Check(((DrumStep)((HardwarePattern)level.After).Tracks[3].Steps[0]).Lanes[4].Level==HitLevel.Ghost);
    }),
    ("unchanged hardware edits preserve identity and bypass locks without history",()=> {
        var p=Hardware();
        foreach (var index in new[]{0,3}) {
            var t=p.Tracks[index]; var selected=HSelect(t,0,1);
            var edits=index==0 ? new[]{HardwareEdit.Transpose(0),HardwareEdit.SetLevel(HitLevel.Soft)} : new[]{HardwareEdit.Variation(1,0)};
            foreach(var edit in edits) {
                var proposal=HardwareEditor.Propose(p,selected,edit,new([t.Id],[t.Steps[0].Id]));
                Check(proposal.Changes.IsEmpty);
                var h=new EditHistory(p); h.Apply(proposal); Check(!h.CanUndo && ReferenceEquals(h.Current,p));
            }
        }
        var drum=p.Tracks[3]; var empty=HardwareEditor.Propose(p,HSelect(drum,1,2),HardwareEdit.SetLevel(HitLevel.Normal),new([],[drum.Steps[1].Id]));
        Check(empty.Changes.IsEmpty);
        Reject(()=>HardwareEditor.Propose(p,HSelect(p.Tracks[0],0,1),HardwareEdit.Transpose(1),new([p.Tracks[0].Id],[])));
    }),
    ("app move retains pitch, duration, identity and other parts",()=> {
        var p=App(); var n=p.Notes[0]; var proposal=AppEditor.Propose(p,Select(n),AppEdit.Move(new(120)));
        var after=(AppPattern)proposal.After;
        Check(after.Notes[0]==n with { Start=new(133) }); Check(after.Notes[1]==p.Notes[1]);
        Check(proposal.Changes.Single() is NoteChange { Before:not null, After:not null });
    }),
    ("app duration, transpose, velocity and quantize",()=> {
        var p=App(); var n=p.Notes[0];
        Check(((AppPattern)AppEditor.Propose(p,Select(n),AppEdit.Resize(new(100))).After).Notes[0].Duration.Value==340);
        Check(((AppPattern)AppEditor.Propose(p,Select(n),AppEdit.Transpose(12)).After).Notes[0].Pitch==72);
        Check(((AppPattern)AppEditor.Propose(p,Select(n),AppEdit.Velocity(-20)).After).Notes[0].Velocity==80);
        Check(((AppPattern)AppEditor.Propose(p,Select(n),AppEdit.Quantize(new(120))).After).Notes[0].Start.Value==0);
    }),
    ("app duplicate has new identity and undo restores references",()=> {
        var p=App(); var history=new EditHistory(p);
        var proposal=AppEditor.Propose(p,Select(p.Notes[0]),AppEdit.Duplicate(new(960))); history.Apply(proposal);
        var a=(AppPattern)history.Current; Check(a.Notes.Length==3); Check(a.Notes[0].Id!=a.Notes[1].Id);
        var accepted=a.Notes; history.Undo(); Check(((AppPattern)history.Current).Notes==p.Notes);
        history.Redo(); Check(((AppPattern)history.Current).Notes==accepted);
        history.Undo(); history.UndoSafeFailure();
    }),
    ("transaction history stale proposals and branch invalidation",()=> {
        var p=App(); var h=new EditHistory(p);
        var first=AppEditor.Propose(p,Select(p.Notes[0]),AppEdit.Move(new(20)));
        var stale=AppEditor.Propose(p,Select(p.Notes[1]),AppEdit.Transpose(1));
        h.Apply(first); Reject(()=>h.Apply(stale)); h.Undo(); Reject(()=>h.Apply(first));
        var current=(AppPattern)h.Current; h.Apply(AppEditor.Propose(current,Select(current.Notes[0]),AppEdit.Velocity(-1)));
        Check(!h.CanRedo); h.Undo(); Check(((AppPattern)h.Current).Notes==p.Notes);
    }),
    ("app boundaries invalid edit atomicity and locked parts",()=> {
        var p=App(); var n=p.Notes[0];
        Reject(()=>AppEditor.Propose(p,Select(n),AppEdit.Move(new(-14))));
        Reject(()=>AppEditor.Propose(p,Select(n),AppEdit.Resize(new(-240))));
        Reject(()=>AppEditor.Propose(p,Select(n),AppEdit.Transpose(100)));
        Reject(()=>AppEditor.Propose(p,Select(n),AppEdit.Velocity(28)));
        Reject(()=>AppEditor.Propose(p,Select(n),AppEdit.Quantize(new(0))));
        Reject(()=>AppEditor.Propose(p,Select(n),AppEdit.Move(new(long.MaxValue))));
        Reject(()=>AppEditor.Propose(p,Select(n),AppEdit.Transpose(1),new([n.PartId],[])));
        Reject(()=>AppEditor.Propose(p,new([Guid.NewGuid()]),AppEdit.Transpose(1)));
        Check(p.Notes[0]==n);
    }),
    ("half-open ranges and long frame positions",()=> {
        var r=new TickRange(new(10),new(20)); r.Validate(); Check(r.Contains(new(10)) && !r.Contains(new(20)));
        Check(r.Overlaps(new(5),new(6)) && !r.Overlaps(new(0),new(10)));
        new FrameRange(new(4_000_000_000),new(4_000_000_001)).Validate();
        Reject(()=>new FrameRange(new(1),new(1)).Validate());
        var p=App(); var a=AppEditor.Propose(p,new(p.Notes.Select(n=>n.Id).ToImmutableHashSet(),new(new(0),new(13))),AppEdit.Transpose(1));
        Check(a.Changes.Length==0);
    }),
    ("selected app variation preserves excluded melody and is repeatable",()=> {
        var p=App(); var a=AppEditor.Propose(p,AppSelection.All(p),AppEdit.Variation(2,0));
        var b=AppEditor.Propose(p,AppSelection.All(p),AppEdit.Variation(2,0));
        Check(((AppPattern)a.After).Notes.SequenceEqual(((AppPattern)b.After).Notes));
        var one=AppEditor.Propose(p,Select(p.Notes[0]),AppEdit.Variation(2,0)); Check(((AppPattern)one.After).Notes.Contains(p.Notes[1]));
    }),
    ("hardware transposition preserves inactive slots, flags, ties, locks and other tracks",()=> {
        var p=Hardware(); var t=p.Tracks[0]; var step=(SynthStep)t.Steps[0];
        var a=(HardwarePattern)HardwareEditor.Propose(p,HSelect(t,0,1),HardwareEdit.Transpose(12)).After;
        var s=(SynthStep)a.Tracks[0].Steps[0]; Check(s.Slots[0]==step.Slots[0] with { Pitch=72 });
        Check(s.Slots[3]==step.Slots[3]); Check(s.Micro==step.Micro && s.Fill==step.Fill && s.Flags==step.Flags && s.Velocity==step.Velocity);
        Check(a.Tracks[0].Steps[1]==t.Steps[1]); Check(a.Tracks[0].Locks==t.Locks); Check(a.Tracks[1]==p.Tracks[1]);
    }),
    ("drum variation keeps kick and native inactive data identical",()=> {
        var p=Hardware(); var t=p.Tracks[3]; var kick=((DrumStep)t.Steps[0]).Lanes[0];
        var a=HardwareEditor.Propose(p,HSelect(t,0,4) with { DrumLanes=[4] },HardwareEdit.Variation(2,1));
        var next=(HardwarePattern)a.After;
        Check(((DrumStep)next.Tracks[3].Steps[0]).Lanes[0]==kick);
        Check(!((DrumStep)next.Tracks[3].Steps[0]).Lanes[4].On);
        Check(((DrumStep)next.Tracks[3].Steps[0]).Lanes[4].Level==HitLevel.Hard);
        var h=new EditHistory(p); h.Apply(a); h.Undo(); Check(((HardwarePattern)h.Current).Tracks==p.Tracks); h.Redo(); Check(((HardwarePattern)h.Current).Tracks==next.Tracks);
    }),
    ("hardware complete move preserves note and parameter-lock IDs",()=> {
        var p=Hardware(); var t=p.Tracks[0];
        var a=(HardwarePattern)HardwareEditor.Propose(p,HSelect(t,0,2),HardwareEdit.Move(new(4))).After;
        Check(a.Tracks[0].Steps[4]==t.Steps[0]); Check(a.Tracks[0].Steps[5]==t.Steps[1]);
        Check(a.Tracks[0].Locks[0]==t.Locks[0] with { Step=new(4) }); PatternValidation.Validate(a);
        var copy=(HardwarePattern)HardwareEditor.Propose(p,HSelect(t,0,2),HardwareEdit.Duplicate(new(4))).After;
        Check(copy.Tracks[0].Steps[4].Id!=t.Steps[0].Id); Check(copy.Tracks[0].Locks.Length==2); PatternValidation.Validate(copy);
    }),
    ("hardware rejects overwrites, overflow, locked events and unsupported representations",()=> {
        var p=Hardware(); var t=p.Tracks[0];
        Reject(()=>HardwareEditor.Propose(p,HSelect(t,0,1),HardwareEdit.Move(new(1))));
        Reject(()=>HardwareEditor.Propose(p,HSelect(t,0,2),HardwareEdit.Move(new(63))));
        Reject(()=>HardwareEditor.Propose(p,HSelect(t,0,1),HardwareEdit.Resize(new(1))));
        Reject(()=>HardwareEditor.Propose(p,HSelect(t,0,1),HardwareEdit.Transpose(100)));
        Reject(()=>HardwareEditor.Propose(p,HSelect(t,0,1),HardwareEdit.Velocity(28)));
        Reject(()=>HardwareEditor.Propose(p,HSelect(t,0,1),HardwareEdit.Transpose(1),new([], [((SynthStep)t.Steps[0]).Slots[0].Id])));
        Reject(()=>HardwareEditor.Propose(p,HSelect(p.Tracks[3],0,1),HardwareEdit.Velocity(1)));
        Reject(()=>HardwareEditor.Propose(p,HSelect(p.Tracks[3],0,1) with { DrumLanes=[4] },HardwareEdit.QuantizeMicro()));
    }),
    ("tie and slide chains across selection and loop boundaries reject atomic move",()=> {
        var p=Hardware(); var t=p.Tracks[0];
        Reject(()=>HardwareEditor.Propose(p,HSelect(t,0,1),HardwareEdit.Move(new(4))));
        Reject(()=>HardwareEditor.Propose(p,HSelect(t,1,2),HardwareEdit.Move(new(4))));
        var wrap=t with { Steps=t.Steps.SetItem(63,((SynthStep)t.Steps[63]) with { Flags=StepFlags.Slide }) };
        Reject(()=>HardwareEditor.Propose(p with { Tracks=p.Tracks.SetItem(0,wrap) },HSelect(wrap,0,2),HardwareEdit.Move(new(4))));
        var dest=t with { Steps=t.Steps.SetItem(6,((SynthStep)t.Steps[6]) with { Time=StepTime.Tie }) };
        Reject(()=>HardwareEditor.Propose(p with { Tracks=p.Tracks.SetItem(0,dest) },HSelect(dest,0,2),HardwareEdit.Move(new(4))));
    }),
    ("native step level edits and micro quantize preserve unrelated fields",()=> {
        var p=Hardware(); var t=p.Tracks[0]; var s=(SynthStep)t.Steps[0];
        var level=(HardwarePattern)HardwareEditor.Propose(p,HSelect(t,0,1),HardwareEdit.SetLevel(HitLevel.Hard)).After;
        Check(((SynthStep)level.Tracks[0].Steps[0]).Slots[0].Level==HitLevel.Hard);
        Check(((SynthStep)level.Tracks[0].Steps[0]).Slots[3]==s.Slots[3]);
        var quant=(HardwarePattern)HardwareEditor.Propose(p,HSelect(t,0,1),HardwareEdit.QuantizeMicro()).After;
        Check(quant.Tracks[0].Steps[0]==s with { Micro=new(0) });
    }),
    ("undo and redo multiple accepted operations preserve full state",()=> {
        var p=App(); var h=new EditHistory(p); var ids=p.Notes.Select(n=>n.Id).ToArray();
        h.Apply(AppEditor.Propose(p,Select(p.Notes[0]),AppEdit.Transpose(7)));
        var second=(AppPattern)h.Current;
        h.Apply(AppEditor.Propose(second,Select(second.Notes[1]),AppEdit.Resize(new(100))));
        var final=(AppPattern)h.Current; Check(h.AcceptedHistory.Length==2);
        h.Undo(); Check(((AppPattern)h.Current).Notes==second.Notes); h.Undo(); Check(((AppPattern)h.Current).Notes==p.Notes);
        h.Redo(); h.Redo(); Check(((AppPattern)h.Current).Notes==final.Notes); Check(((AppPattern)h.Current).Notes.Select(n=>n.Id).SequenceEqual(ids));
    }),    ("compound gesture applies once and restores every component",()=> {
        var p=App(); var selection=Select(p.Notes[0]);
        var pitch=AppEditor.Propose(p,selection,AppEdit.Transpose(7));
        var move=AppEditor.Propose((AppPattern)pitch.After,selection,AppEdit.Move(new(100)));
        var compound=EditProposal.Compose([pitch,move],new("Pitch and onset gesture","manual/1"));
        Check(compound.Changes.Length==1); var h=new EditHistory(p); h.Apply(compound); Check(h.AcceptedHistory.Length==1);
        h.Undo(); Check(((AppPattern)h.Current).Notes==p.Notes); h.Redo(); Check(((AppPattern)h.Current).Notes==((AppPattern)move.After).Notes);
        Reject(()=>EditProposal.Compose([move,pitch],new("Invalid chain","manual/1")));
    }),    ("hardware validation capacities, duplicate lock keys, engine ranges and native timing",()=> {
        var p=Hardware(); var t=p.Tracks[0];
        Reject(()=>PatternValidation.Validate(p with { Tracks=p.Tracks.RemoveAt(3) }));
        Reject(()=>PatternValidation.Validate(p with { Tracks=p.Tracks.SetItem(0,t with { Length=65 }) }));
        Reject(()=>PatternValidation.Validate(p with { Tracks=p.Tracks.SetItem(0,t with { Locks=t.Locks.Add(t.Locks[0] with { Id=Guid.NewGuid() }) }) }));
        Reject(()=>PatternValidation.Validate(p with { Tracks=p.Tracks.SetItem(0,t with { Locks=[t.Locks[0] with { Parameter=53,Value=999 }] }) }));
        Reject(()=>PatternValidation.Validate(p with { Tracks=p.Tracks.SetItem(0,t with { Locks=[t.Locks[0] with { Parameter=29 }] }) }));
        Reject(()=>PatternValidation.Validate(p with { Tracks=p.Tracks.SetItem(0,t with { Steps=t.Steps.SetItem(0,((SynthStep)t.Steps[0]) with { Micro=new(32) }) }) }));
        var full=t with { Locks=Enumerable.Range(0,24).Select(i=>new ParameterLock(Guid.NewGuid(),new(i),0,50)).ToImmutableArray() };
        var fullPattern=p with { Tracks=p.Tracks.SetItem(0,full) };
        Reject(()=>HardwareEditor.Propose(fullPattern,HSelect(full,0,1),HardwareEdit.Duplicate(new(30))));
        Check(HardwareCapabilities.CurrentFirmwareLockable.Contains(60) && !HardwareCapabilities.CurrentFirmwareLockable.Contains(51));
    }),
};
int failures=0;
foreach(var (name,test) in tests) { try { test(); Console.WriteLine($"PASS {name}"); } catch(Exception ex) { failures++; Console.Error.WriteLine($"FAIL {name}: {ex}"); } }
Console.WriteLine($"{tests.Length-failures}/{tests.Length} passed"); return failures==0 ? 0 : 1;
static void Check(bool yes) { if(!yes) throw new Exception("Assertion failed."); }
static void Reject(Action operation) { try { operation(); } catch(EditException) { return; } catch(OverflowException) { return; } throw new Exception("Expected edit rejection."); }
static AppSelection Select(AppNote n)=>new([n.Id]);
static HardwareSelection HSelect(HardwareTrack t,int start,int end)=>new(t.Id,new(new(start),new(end)));
static AppPattern App()=>new(Guid.NewGuid(),Guid.NewGuid(),960,new(3840),[
    new(Guid.NewGuid(),Guid.NewGuid(),new(13),new(240),60,100,0),
    new(Guid.NewGuid(),Guid.NewGuid(),new(960),new(480),67,90,1)]);
static HardwarePattern Hardware()
{
    var cap=new HardwareCapabilities("checked-out-core.h/engine-0",HardwareCapabilities.CurrentFirmwareLockable,
        new Dictionary<int,ParameterRange> { [0]=new(0,100),[53]=new(0,7) }.ToImmutableDictionary());
    var tracks=Enumerable.Range(0,4).Select(i=>new HardwareTrack(Guid.NewGuid(),i==3,64,
        Enumerable.Range(0,64).Select(_=>HardwareEditor.Blank(i==3)).ToImmutableArray(),[],ImmutableDictionary<int,int>.Empty,cap)).ToImmutableArray();
    var t=tracks[0]; var s=(SynthStep)t.Steps[0];
    s=s with { Count=1,Time=StepTime.Note,Velocity=100,Flags=StepFlags.Slide|StepFlags.Accent,Micro=new(-32),Fill=FillCondition.FillOnly,
        Slots=s.Slots.SetItem(0,s.Slots[0] with { Pitch=60,Level=HitLevel.Soft,Ratchet=3 }).SetItem(3,s.Slots[3] with { Pitch=99,Level=HitLevel.Ghost,Ratchet=4 }) };
    t=t with { Steps=t.Steps.SetItem(0,s).SetItem(1,((SynthStep)t.Steps[1]) with { Time=StepTime.Tie }),Locks=[new(Guid.NewGuid(),new(0),0,75)] };
    tracks=tracks.SetItem(0,t); var d=tracks[3]; var ds=(DrumStep)d.Steps[0];
    ds=ds with { Lanes=ds.Lanes.SetItem(0,ds.Lanes[0] with { On=true }).SetItem(4,ds.Lanes[4] with { On=true,Level=HitLevel.Hard,Ratchet=4 }) };
    return new(Guid.NewGuid(),Guid.NewGuid(),tracks.SetItem(3,d with { Steps=d.Steps.SetItem(0,ds) }));
}
static class HistoryTestExtensions { public static void UndoSafeFailure(this EditHistory h) { try { h.Undo(); } catch(EditException) { return; } throw new Exception("Undo should be empty."); } }



