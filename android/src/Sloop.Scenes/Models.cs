using System.Collections.Immutable;
using Sloop.Sequencing;
using Sloop.SoundDesign;

namespace Sloop.Scenes;

public sealed record SceneSound(string TargetId, SoundState State);
public sealed record NativePatternIdentity(Guid Id, Guid Revision, string DeviceIdentity, int Bank, int Slot);
/// <summary>App-owned immutable snapshot; native references do not imply stored device bytes.</summary>
public sealed record SceneSnapshot(Guid Id, Guid Revision, string Name, int Tempo,
    ImmutableArray<SceneSound> Sounds, AppPattern? AppPattern,
    ImmutableArray<NativePatternIdentity> NativePatterns)
{
    public void Validate()
    {
        if (Id == Guid.Empty || Revision == Guid.Empty || string.IsNullOrWhiteSpace(Name) || Name.Length > 128 ||
            Tempo is < 30 or > 240 || Sounds.IsDefault || NativePatterns.IsDefault)
            throw new ArgumentException("Invalid scene identity, name, tempo or collection.");
        var targets = new HashSet<string>(StringComparer.Ordinal);
        foreach (var sound in Sounds)
        {
            if (string.IsNullOrWhiteSpace(sound.TargetId) || !targets.Add(sound.TargetId))
                throw new ArgumentException("Sound targets must be unique and nonempty.");
            PatchValidation.Require(sound.State.Patch); sound.State.Macros.Validate();
        }
        if (AppPattern is not null) PatternValidation.Validate(AppPattern);
        var slots = new HashSet<(string, int, int)>();
        foreach (var p in NativePatterns)
            if (p.Id == Guid.Empty || p.Revision == Guid.Empty || string.IsNullOrWhiteSpace(p.DeviceIdentity) ||
                p.Bank < 0 || p.Slot < 0 || !slots.Add((p.DeviceIdentity, p.Bank, p.Slot)))
                throw new ArgumentException("Invalid or duplicate native pattern reference.");
    }
}
public sealed record SceneStep(Guid SceneId, int Repeats);
public sealed record Arrangement(Guid Id, string Name, ImmutableArray<SceneStep> Steps, bool Loop)
{
    public void Validate(IReadOnlyDictionary<Guid, SceneSnapshot> scenes)
    {
        if (Id == Guid.Empty || string.IsNullOrWhiteSpace(Name) || Steps.IsDefaultOrEmpty || Steps.Length > 4096)
            throw new ArgumentException("Invalid arrangement.");
        foreach (var step in Steps)
            if (step.Repeats is < 1 or > 1024 || !scenes.ContainsKey(step.SceneId))
                throw new ArgumentException("Arrangement has an invalid repeat count or missing scene.");
    }
}
/// <summary>Advance once per completed scene phrase, never per UI render or wall-clock timer.</summary>
public sealed class ArrangementCursor
{
    public Arrangement Arrangement { get; }
    public int Index { get; private set; }
    public int Repeat { get; private set; }
    public bool Complete { get; private set; }
    public Guid? Current => Complete ? null : Arrangement.Steps[Index].SceneId;
    public ArrangementCursor(Arrangement arrangement, IReadOnlyDictionary<Guid, SceneSnapshot> scenes)
    { arrangement.Validate(scenes); Arrangement = arrangement; }
    public Guid? Advance()
    {
        if (Complete) return null;
        if (++Repeat < Arrangement.Steps[Index].Repeats) return Current;
        Repeat = 0;
        if (++Index == Arrangement.Steps.Length)
        { if (Arrangement.Loop) Index = 0; else { Complete = true; return null; } }
        return Current;
    }
}
