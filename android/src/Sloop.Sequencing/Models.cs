using System.Collections.Immutable;
namespace Sloop.Sequencing;

public readonly record struct Tick(long Value);
public readonly record struct Frame(long Value);
public readonly record struct StepIndex(int Value);
public readonly record struct StepOffset(int Value);
/// <summary>Native units: 1/64 step. No implicit conversion to ticks or frames.</summary>
public readonly record struct MicroOffset(int Value);
public readonly record struct TickRange(Tick Start, Tick End)
{
    public bool Contains(Tick position) => position.Value >= Start.Value && position.Value < End.Value;
    public bool Overlaps(Tick start, Tick duration) => start.Value < End.Value && checked(start.Value + duration.Value) > Start.Value;
    public void Validate() { if (Start.Value < 0 || End.Value <= Start.Value) throw new EditException("Range must be nonempty and half-open."); }
}
public readonly record struct FrameRange(Frame Start, Frame End)
{
    public void Validate() { if (Start.Value < 0 || End.Value <= Start.Value) throw new EditException("Invalid frame range."); }
}
public readonly record struct StepRange(StepIndex Start, StepIndex End)
{
    public bool Contains(int step) => step >= Start.Value && step < End.Value;
    public void Validate() { if (Start.Value < 0 || End.Value > 64 || End.Value <= Start.Value) throw new EditException("Invalid step range."); }
}
public sealed class EditException(string message) : Exception(message);
public abstract record Pattern(Guid Id, Guid Revision);
public sealed record AppNote(Guid Id, Guid PartId, Tick Start, Tick Duration, int Pitch, int Velocity, int Channel);
public sealed record AppPattern(Guid Id, Guid Revision, int TicksPerQuarter, Tick Length,
    ImmutableArray<AppNote> Notes) : Pattern(Id, Revision);
public enum StepTime { Note, Tie, Rest }
[Flags] public enum StepFlags { None = 0, Accent = 1, Slide = 2 }
public enum HitLevel { Normal, Ghost, Soft, Hard }
public enum FillCondition { Normal, FillOnly, NoFill, ReservedNormal }
/// <summary>All four physical slots retained, including inactive slot data. Count on SynthStep selects active slots.</summary>
public sealed record SynthSlot(Guid Id, int Pitch, HitLevel Level, int Ratchet);
public abstract record HardwareStep(Guid Id, MicroOffset Micro, FillCondition Fill);
public sealed record SynthStep(Guid Id, MicroOffset Micro, FillCondition Fill, ImmutableArray<SynthSlot> Slots,
    int Count, StepTime Time, StepFlags Flags, int Velocity) : HardwareStep(Id, Micro, Fill);
/// <summary>All sixteen lanes retained, including level/ratchet bits of inactive lanes.</summary>
public sealed record DrumHit(Guid Id, bool On, HitLevel Level, int Ratchet);
public sealed record DrumStep(Guid Id, MicroOffset Micro, FillCondition Fill, ImmutableArray<DrumHit> Lanes) : HardwareStep(Id, Micro, Fill);
public sealed record ParameterLock(Guid Id, StepIndex Step, int Parameter, int Value);
public sealed record ParameterRange(int Minimum, int Maximum);
/// <summary>Snapshot of the exact parameter layout and engine-specific ranges. Unknown ranges fail closed.</summary>
public sealed record HardwareCapabilities(string Identity, ImmutableHashSet<int> LockableParameters,
    ImmutableDictionary<int, ParameterRange> ParameterRanges)
{
    public static ImmutableHashSet<int> CurrentFirmwareLockable { get; } =
        Enumerable.Range(0,17).Concat(new[] {32,33,34,35,36,38,39,44,45,46,47,48,50})
        .Concat(Enumerable.Range(53,8)).ToImmutableHashSet();
}
public sealed record HardwareTrack(Guid Id, bool IsDrum, int Length, ImmutableArray<HardwareStep> Steps,
    ImmutableArray<ParameterLock> Locks, ImmutableDictionary<int,int> Parameters, HardwareCapabilities Capabilities);
public sealed record HardwarePattern(Guid Id, Guid Revision, ImmutableArray<HardwareTrack> Tracks) : Pattern(Id, Revision);
public sealed record AppSelection(ImmutableHashSet<Guid> NoteIds, TickRange? Range = null)
{
    public static AppSelection All(AppPattern pattern) => new(pattern.Notes.Select(n => n.Id).ToImmutableHashSet());
    internal bool Includes(AppNote n) => NoteIds.Contains(n.Id) && (Range is null || Range.Value.Contains(n.Start));
}
public sealed record HardwareSelection(Guid TrackId, StepRange Range, ImmutableHashSet<int>? DrumLanes = null);
public sealed record EditLocks(ImmutableHashSet<Guid> Parts, ImmutableHashSet<Guid> Events)
{
    public static EditLocks None { get; } = new(ImmutableHashSet<Guid>.Empty, ImmutableHashSet<Guid>.Empty);
}

