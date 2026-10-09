using System.Collections.Immutable;
namespace Sloop.Sequencing;
public enum HardwareEditKind { MoveSteps, DuplicateSteps, Transpose, Velocity, SetLevel, QuantizeMicro, Variation, Resize }
public sealed record HardwareEdit
{
    public HardwareEditKind Kind { get; }
    internal int Amount { get; }
    public int Seed { get; }
    private HardwareEdit(HardwareEditKind kind,int amount=0,int seed=0) { Kind=kind; Amount=amount; Seed=seed; }
    public static HardwareEdit Move(StepOffset delta) => new(HardwareEditKind.MoveSteps,delta.Value);
    public static HardwareEdit Duplicate(StepOffset offset) => new(HardwareEditKind.DuplicateSteps,offset.Value);
    public static HardwareEdit Transpose(int semitones) => new(HardwareEditKind.Transpose,semitones);
    public static HardwareEdit Velocity(int delta) => new(HardwareEditKind.Velocity,delta);
    public static HardwareEdit SetLevel(HitLevel level) => new(HardwareEditKind.SetLevel,(int)level);
    public static HardwareEdit QuantizeMicro() => new(HardwareEditKind.QuantizeMicro);
    public static HardwareEdit Variation(int retentionDivisor,int seed) => new(HardwareEditKind.Variation,retentionDivisor,seed);
    /// <summary>Explicitly rejected because firmware has no independent note duration.</summary>
    public static HardwareEdit Resize(Tick delta) => new(HardwareEditKind.Resize);
}
public static class HardwareEditor
{
    public static EditProposal Propose(HardwarePattern source, HardwareSelection selection, HardwareEdit edit, EditLocks? locks = null, string? prompt = null)
    {
        PatternValidation.Validate(source); selection.Range.Validate(); locks ??= EditLocks.None;
        int trackIndex = source.Tracks.FindIndex(t=>t.Id==selection.TrackId);
        if (trackIndex < 0) throw new EditException("Unknown track selection.");
        var track = source.Tracks[trackIndex];
        if (selection.DrumLanes is { } lanes && (!track.IsDrum || lanes.Any(l=>l is < 0 or > 15))) throw new EditException("Invalid drum lane selection.");
        if (edit.Kind == HardwareEditKind.Resize) throw new EditException("Firmware has step-wide ties/gate, not independent note durations. Use app notes or explicit step/tie edits.");
        if (track.IsDrum && edit.Kind is HardwareEditKind.Transpose or HardwareEditKind.Velocity)
            throw new EditException("Drum lanes have native hit levels, not editable MIDI pitch/velocity. Use SetLevel.");
        if (edit.Kind == HardwareEditKind.Variation && edit.Amount < 1) throw new EditException("Variation retention divisor must be positive.");
        if (edit.Kind == HardwareEditKind.SetLevel && edit.Amount is < 0 or > 3) throw new EditException("Native level must be 0..3.");
        var steps = track.Steps.ToBuilder(); var plocks = track.Locks.ToBuilder();
        if (edit.Kind is HardwareEditKind.MoveSteps or HardwareEditKind.DuplicateSteps)
        {
            if (selection.DrumLanes is not null) throw new EditException("Lane-only move/copy cannot preserve step-wide timing/conditions/locks. Select whole steps.");
            if (edit.Amount == 0) throw new EditException("Move/copy offset must be nonzero.");
                        // A tie or slide crossing a selection boundary would change phrase articulation.
                            int start=selection.Range.Start.Value, end=selection.Range.End.Value;
                if(end>track.Length) throw new EditException("Move/copy source exceeds active phrase length.");
                int destinationStart=checked(start+edit.Amount), destinationEnd=checked(end+edit.Amount);
                if(destinationStart<0 || destinationEnd>track.Length) throw new EditException("Destination exceeds active phrase length.");
            if (!track.IsDrum)
            {
                var first=(SynthStep)track.Steps[start];
                var previous=(SynthStep)track.Steps[(start+track.Length-1)%track.Length];
                var last=(SynthStep)track.Steps[end-1];
                                var following=(SynthStep)track.Steps[end%track.Length];
                var destinationPrevious=(SynthStep)track.Steps[(destinationStart+track.Length-1)%track.Length];
                var destinationFollowing=(SynthStep)track.Steps[destinationEnd%track.Length];
                if(destinationPrevious.Flags.HasFlag(StepFlags.Slide) || destinationFollowing.Time==StepTime.Tie)
                    throw new EditException("Destination borders a tie/slide chain; moving would alter its articulation.");
                if(first.Time==StepTime.Tie || previous.Flags.HasFlag(StepFlags.Slide) || last.Flags.HasFlag(StepFlags.Slide) || following.Time==StepTime.Tie)
                    throw new EditException("Selection crosses a tie/slide chain, including loop wrap. Select a complete phrase.");
            }
            var indices = Enumerable.Range(selection.Range.Start.Value, selection.Range.End.Value-selection.Range.Start.Value).ToArray();
            foreach (var i in indices)
            {
                int destination = checked(i+edit.Amount);
                if (destination < 0 || destination >= track.Length) throw new EditException("Destination exceeds active pattern length; no wrapping or truncation.");
                if (selection.Range.Contains(destination)) throw new EditException("Destination overlaps source; choose a separate phrase range.");
                if (!selection.Range.Contains(destination) && (!Empty(track.Steps[destination]) || track.Locks.Any(l=>l.Step.Value==destination)))
                    throw new EditException("Destination contains data; refusing to overwrite it.");
                Guard(track.Steps[i],locks); Guard(track.Steps[destination],locks);
                foreach (var l in track.Locks.Where(l=>l.Step.Value==i)) if (locks.Events.Contains(l.Id)) throw new EditException("Parameter lock is protected.");
            }
            if (edit.Kind == HardwareEditKind.MoveSteps)
            {
                foreach(var i in indices) steps[i] = Blank(track.IsDrum);
                for (int l=0;l<plocks.Count;l++) if(selection.Range.Contains(plocks[l].Step.Value)) plocks[l]=plocks[l] with { Step = new(checked(plocks[l].Step.Value+edit.Amount)) };
            }
            foreach (var i in indices)
            {
                steps[i+edit.Amount] = edit.Kind == HardwareEditKind.MoveSteps ? track.Steps[i] : Clone(track.Steps[i]);
                if (edit.Kind == HardwareEditKind.DuplicateSteps)
                    foreach (var l in track.Locks.Where(l=>l.Step.Value==i)) plocks.Add(l with { Id=Guid.NewGuid(), Step=new(i+edit.Amount) });
            }
        }
        else
        {
            for(int i=selection.Range.Start.Value;i<selection.Range.End.Value;i++)
            {
                var old = steps[i]; HardwareStep next = old;
                if (edit.Kind == HardwareEditKind.QuantizeMicro)
                {
                    if(selection.DrumLanes is not null) throw new EditException("Micro timing applies to every lane; select the whole step.");
                    next = old switch { SynthStep s=>s with { Micro=new(0) }, DrumStep d=>d with { Micro=new(0) }, _=>old };
                }
                else if (old is SynthStep synth)
                {
                    var slots=synth.Slots.ToBuilder();
                    switch(edit.Kind)
                    {
                        case HardwareEditKind.Transpose:
                            for(int n=0;n<synth.Count;n++) slots[n]=slots[n] with { Pitch=checked(slots[n].Pitch+edit.Amount) };
                            next=slots.SequenceEqual(synth.Slots) ? synth : synth with { Slots=slots.ToImmutable() }; break;
                        case HardwareEditKind.Velocity: next=synth with { Velocity=checked(synth.Velocity+edit.Amount) }; break;
                        case HardwareEditKind.SetLevel:
                            for(int n=0;n<synth.Count;n++) slots[n]=slots[n] with { Level=(HitLevel)edit.Amount };
                            next=slots.SequenceEqual(synth.Slots) ? synth : synth with { Slots=slots.ToImmutable() }; break;
                        case HardwareEditKind.Variation: throw new EditException("Synth thinning can break step-wide ties/slides. Use app-note variation or explicit step edits.");
                        default: throw new EditException("Unsupported hardware edit.");
                    }
                }
                else if(old is DrumStep drum)
                {
                    var hits=drum.Lanes.ToBuilder();
                    for(int lane=0;lane<16;lane++)
                    {
                        if(selection.DrumLanes is not null && !selection.DrumLanes.Contains(lane)) continue;
                        var hit=hits[lane]; if(!hit.On) continue;
                        hits[lane]=edit.Kind switch
                        {
                            HardwareEditKind.SetLevel=>hit with { Level=(HitLevel)edit.Amount },
                            HardwareEditKind.Variation=>hit with { On= ((long)i+lane+edit.Seed)%edit.Amount==0 },
                            _=>throw new EditException("Unsupported drum edit.")
                        };
                        if(hits[lane]!=hit && locks.Events.Contains(hit.Id)) throw new EditException("Drum hit is locked.");
                    }
                    next=hits.SequenceEqual(drum.Lanes) ? drum : drum with { Lanes=hits.ToImmutable() };
                }
                if (next!=old) GuardChanges(old,next,locks);
                steps[i]=next;
            }
        }
                var editedSteps=steps.SequenceEqual(track.Steps) ? track.Steps : steps.ToImmutable();
        var editedLocks=plocks.SequenceEqual(track.Locks) ? track.Locks : plocks.ToImmutable();
        var edited=track with { Steps=editedSteps, Locks=editedLocks };
        if (edited != track && locks.Parts.Contains(track.Id)) throw new EditException("Selected part is locked.");
        var after=source with { Revision=Guid.NewGuid(), Tracks=source.Tracks.SetItem(trackIndex,edited) };
        PatternValidation.Validate(after);
        return new(source,after,new(edit.Kind.ToString(),"sequencing-rules/1",edit.Kind==HardwareEditKind.Variation ? edit.Seed : null,prompt));
    }
    private static bool Empty(HardwareStep step) => step.Micro.Value==0 && step.Fill==FillCondition.Normal && step switch
    {
        SynthStep s=>s.Count==0 && s.Time==StepTime.Rest && s.Flags==StepFlags.None && s.Velocity==0 && s.Slots.All(n=>n.Pitch==0 && n.Level==HitLevel.Normal && n.Ratchet==1),
        DrumStep d=>d.Lanes.All(l=>!l.On && l.Level==HitLevel.Normal && l.Ratchet==1), _=>false
    };
    private static void Guard(HardwareStep step, EditLocks locks)
    {
        if(locks.Events.Contains(step.Id) || step is SynthStep s && s.Slots.Any(n=>locks.Events.Contains(n.Id)) || step is DrumStep d && d.Lanes.Any(n=>locks.Events.Contains(n.Id)))
            throw new EditException("Step or contained event is locked.");
    }
    private static void GuardChanges(HardwareStep old,HardwareStep next,EditLocks locks)
    {
        if(locks.Events.Contains(old.Id)) throw new EditException("Step is locked.");
        if(old is SynthStep a && next is SynthStep b)
            for(int i=0;i<4;i++) if(locks.Events.Contains(a.Slots[i].Id) && (a.Slots[i]!=b.Slots[i] || a.Velocity!=b.Velocity || a.Micro!=b.Micro)) throw new EditException("Synth note is locked.");
        if(old is DrumStep c && next is DrumStep d)
            for(int i=0;i<16;i++) if(locks.Events.Contains(c.Lanes[i].Id) && (c.Lanes[i]!=d.Lanes[i] || c.Micro!=d.Micro)) throw new EditException("Drum hit is locked.");
    }
    public static HardwareStep Blank(bool drum) => drum ?
        new DrumStep(Guid.NewGuid(),new(0),FillCondition.Normal,Enumerable.Range(0,16).Select(_=>new DrumHit(Guid.NewGuid(),false,HitLevel.Normal,1)).ToImmutableArray()) :
        new SynthStep(Guid.NewGuid(),new(0),FillCondition.Normal,Enumerable.Range(0,4).Select(_=>new SynthSlot(Guid.NewGuid(),0,HitLevel.Normal,1)).ToImmutableArray(),0,StepTime.Rest,StepFlags.None,0);
    private static HardwareStep Clone(HardwareStep step) => step switch
    {
        SynthStep s=>s with { Id=Guid.NewGuid(), Slots=s.Slots.Select(n=>n with { Id=Guid.NewGuid() }).ToImmutableArray() },
        DrumStep d=>d with { Id=Guid.NewGuid(), Lanes=d.Lanes.Select(n=>n with { Id=Guid.NewGuid() }).ToImmutableArray() },
        _=>throw new EditException("Unknown step type.")
    };
    private static int FindIndex<T>(this ImmutableArray<T> array,Func<T,bool> match)
    { for(int i=0;i<array.Length;i++) if(match(array[i])) return i; return -1; }
}



