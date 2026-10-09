using System.Collections.Immutable;

namespace Sloop.SoundDesign;

public enum SoundFamily { Bass, Keys, Bell, Pad, Brass, Organ, Pluck, Percussion, Effect }
public enum Character { Low = -1, Template = 0, High = 1 }
public enum PlayingRegister { Low, Middle, High }
public enum VoicePreference { Template, Mono, Poly }
/// <summary>Qualitative recipe controls, not physical units. Template leaves a dimension unchanged.</summary>
public sealed record SoundIntent(SoundFamily Family, uint Seed, Character Brightness = 0,
    Character Harmonicity = 0, Character AttackSpeed = 0, Character DecayLength = 0,
    Character Sustain = 0, Character ReleaseLength = 0, Character VelocityResponse = 0,
    Character Movement = 0, PlayingRegister Register = PlayingRegister.Middle,
    VoicePreference VoicePreference = VoicePreference.Template, int VariationAmount = 25)
{
    public void Validate()
    {
        if (!Enum.IsDefined(Family) || !Enum.IsDefined(Register) || !Enum.IsDefined(VoicePreference)
            || VariationAmount is < 0 or > 100 || new[] { Brightness,Harmonicity,AttackSpeed,DecayLength,
                Sustain,ReleaseLength,VelocityResponse,Movement }.Any(x => !Enum.IsDefined(x)))
            throw new ArgumentException("Intent contains an unsupported value.");
    }
}
public enum RefinementDimension { Brightness, AttackSpeed, DecayLength, Sustain, ReleaseLength, VelocityResponse, Movement, Harmonicity }
public sealed record Refinement(RefinementDimension Dimension, int Amount)
{
    public void Validate()
    {
        if (!Enum.IsDefined(Dimension) || Amount is < -100 or > 100)
            throw new ArgumentException("Refinement amount must be -100..100 in a supported dimension.");
    }
}
[Flags]
public enum LockGroup { None = 0, Algorithm = 1, Tuning = 2, Envelopes = 4, Attack = 8, VoiceControls = 16 }
/// <summary>Hard constraints captured from the parent patch; musical operator numbers 1..6.</summary>
public sealed record PatchLocks(LockGroup Groups, ImmutableHashSet<int> Operators)
{
    public static PatchLocks None { get; } = new(LockGroup.None, ImmutableHashSet<int>.Empty);
    public void Validate()
    {
        if ((Groups & ~(LockGroup.Algorithm|LockGroup.Tuning|LockGroup.Envelopes|LockGroup.Attack|LockGroup.VoiceControls)) != 0
            || Operators is null || Operators.Any(n => n is < 1 or > 6)) throw new ArgumentException("Invalid lock set.");
    }
    public Patch Enforce(Patch before, Patch after)
    {
        Validate();
        if (Groups.HasFlag(LockGroup.VoiceControls)) after=after with {PitchEnvelope=before.PitchEnvelope,
            Algorithm=before.Algorithm,Feedback=before.Feedback,OscillatorSync=before.OscillatorSync,LfoSpeed=before.LfoSpeed,
            LfoDelay=before.LfoDelay,LfoPitchDepth=before.LfoPitchDepth,LfoAmplitudeDepth=before.LfoAmplitudeDepth,
            LfoSync=before.LfoSync,LfoWave=before.LfoWave,PitchSensitivity=before.PitchSensitivity,Transpose=before.Transpose};
        if (Groups.HasFlag(LockGroup.Algorithm)) after = after with { Algorithm = before.Algorithm };
        if (Groups.HasFlag(LockGroup.Tuning)) after = after with { Transpose=before.Transpose, PitchEnvelope=before.PitchEnvelope,
            LfoPitchDepth=before.LfoPitchDepth,PitchSensitivity=before.PitchSensitivity };
        if (Groups.HasFlag(LockGroup.Envelopes)) after=after with { PitchEnvelope=before.PitchEnvelope };
        if (Groups.HasFlag(LockGroup.Attack)) after=after with { PitchEnvelope=after.PitchEnvelope with
            {Rate1=before.PitchEnvelope.Rate1,Level1=before.PitchEnvelope.Level1} };
        for (int n=1;n<=6;n++)
        {
            var b=before.GetOperator(n);var a=after.GetOperator(n);
            if (Groups.HasFlag(LockGroup.Tuning)) a=a with {Mode=b.Mode,Coarse=b.Coarse,Fine=b.Fine,Detune=b.Detune};
            if (Groups.HasFlag(LockGroup.Envelopes)) a=a with {Envelope=b.Envelope};
            if (Groups.HasFlag(LockGroup.Attack)) a=a with {Envelope=a.Envelope with {Rate1=b.Envelope.Rate1,Level1=b.Envelope.Level1}};
            after=after.WithOperator(n,Operators.Contains(n)?b:a);
        }
        return after;
    }
}
/// <summary>Seven sound macros. AlgorithmOverride 0 means PAT. PTCH is deliberately excluded.</summary>
public sealed record MacroContext(int AlgorithmOverride=0,int Feedback=0,int ModulatorLevel=0,
    int ModulatorRatio=0,int ModulatorEnvelope=0,int VelocityModulation=0,int Detune=0)
{
    public static MacroContext Neutral { get; } = new();
    public void Validate()
    {
        if (AlgorithmOverride is < 0 or > 32 || Feedback is < -7 or > 7 || ModulatorLevel is < -64 or > 63
            || ModulatorRatio is < -16 or > 16 || ModulatorEnvelope is < -64 or > 63
            || VelocityModulation is < -7 or > 7 || Detune is < 0 or > 127)
            throw new ArgumentException("Macro context outside firmware ranges.");
    }
}
public sealed record SoundState(Patch Patch, MacroContext Macros);
public sealed record PatchChange(string Field, int Before, int After);
public sealed record AuditionNote(int MidiNote, int Velocity, int StartStep, int LengthSteps);
public sealed record AuditionPlan(int Tempo, ImmutableArray<AuditionNote> Notes);
public sealed record PatchDraft(string TargetId, long ParentRevision, SoundState Before, SoundState After,
    SoundIntent Intent, PatchLocks Locks, uint Seed, string RecipeIdentity, string BasePatchReference,
    string FullName, string Prompt, ImmutableArray<string> Explanation, ImmutableArray<PatchChange> Changes,
    AuditionPlan Audition, string? ParentDraftIdentity=null, ImmutableArray<Refinement> Refinements=default)
{
    public string Identity {
        get {
            // Explicit encoding also works under Android trimming without reflection metadata.
            using var stream=new MemoryStream(); using var writer=new BinaryWriter(stream);
            writer.Write("SLOOP-DRAFT-ID-1");writer.Write(TargetId);writer.Write(ParentRevision);writer.Write(Seed);writer.Write(RecipeIdentity);
            writer.Write(PatchCodec.EncodeVoice(After.Patch));var m=After.Macros;
            foreach(var v in new[]{m.AlgorithmOverride,m.Feedback,m.ModulatorLevel,m.ModulatorRatio,m.ModulatorEnvelope,m.VelocityModulation,m.Detune})writer.Write(v);
            writer.Write(ParentDraftIdentity??"");writer.Write(Prompt);writer.Flush();
            return Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(stream.ToArray()));
        }
    }
    public bool IsNoOp => PatchCodec.ContentEquals(Before.Patch,After.Patch) && Before.Macros==After.Macros;
}
/// <summary>Local transaction only. Hardware adapters must provide their own acknowledgments/recovery.</summary>
public sealed class SoundDocument
{
    readonly Stack<(SoundState State,PatchDraft Draft)> undo=new();
    readonly Stack<(SoundState State,PatchDraft Draft)> redo=new();
    readonly List<PatchDraft> accepted=new();
    public string TargetId { get; }
    public long Revision { get; private set; }
    public SoundState State { get; private set; }
    public ImmutableArray<PatchDraft> AcceptedHistory => accepted.ToImmutableArray();
    public SoundDocument Fork()
    {
        var copy=new SoundDocument(TargetId,State,Revision);
        foreach(var item in undo.Reverse())copy.undo.Push(item);
        foreach(var item in redo.Reverse())copy.redo.Push(item);
        copy.accepted.AddRange(accepted);return copy;
    }
    public SoundDocument(string targetId,SoundState state,long revision=0)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(targetId);
        if(revision<0)throw new ArgumentOutOfRangeException(nameof(revision));
        PatchValidation.Require(state.Patch);state.Macros.Validate();TargetId=targetId;State=state;Revision=revision;
    }
    public void Apply(PatchDraft draft)
    {
        if (draft.TargetId!=TargetId || draft.ParentRevision!=Revision || !Same(State,draft.Before))
            throw new InvalidOperationException("Stale proposal: regenerate against the current target snapshot.");
        PatchValidation.Require(draft.After.Patch);
        draft.After.Macros.Validate();
        if (!PatchCodec.ContentEquals(draft.After.Patch,draft.Locks.Enforce(draft.Before.Patch,draft.After.Patch)))
            throw new ArgumentException("Proposal violates locks.");
        undo.Push((State,draft));redo.Clear();State=draft.After;Revision++;accepted.Add(draft);
    }
    public bool Undo()
    {
        if (!undo.TryPop(out var entry)) return false;
        redo.Push((State,entry.Draft));State=entry.State;Revision++;return true;
    }
    public bool Redo()
    {
        if (!redo.TryPop(out var entry)) return false;
        undo.Push((State,entry.Draft));State=entry.State;Revision++;return true;
    }
    static bool Same(SoundState a,SoundState b) => a.Macros==b.Macros && PatchCodec.ContentEquals(a.Patch,b.Patch);
}
