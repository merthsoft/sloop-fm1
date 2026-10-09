using System.Collections.Immutable;
namespace Sloop.Sequencing;
public abstract record PatternChange;
public sealed record NoteChange(Guid Id, AppNote? Before, AppNote? After) : PatternChange;
/// <summary>Includes all step fields, parameters, locks and capability metadata, including dormant steps.</summary>
public sealed record TrackChange(Guid Id, HardwareTrack Before, HardwareTrack After) : PatternChange;
public sealed record EditProvenance(string Description, string RecipeVersion, int? Seed = null, string? Prompt = null);
public sealed class EditProposal
{
    public Pattern Before { get; }
    public Pattern After { get; }
    public ImmutableArray<PatternChange> Changes { get; }
    public EditProvenance Provenance { get; }
    internal EditProposal(Pattern before, Pattern after, EditProvenance provenance)
    { Before = before; After = after; Provenance = provenance; Changes = Diff(before,after); }
    public static EditProposal Between(Pattern before,Pattern after,EditProvenance provenance)
    {
        PatternValidation.Validate(before); PatternValidation.Validate(after);
        if(before.Id!=after.Id||before.GetType()!=after.GetType())throw new EditException("Snapshots belong to different patterns.");
        return new(before,after,provenance);
    }
    /// <summary>Combine a sequential preview chain into one atomic local edit/gesture. No state is applied here.</summary>
    public static EditProposal Compose(IEnumerable<EditProposal> chain, EditProvenance provenance)
    {
        var entries=chain.ToArray();
        if(entries.Length==0) throw new EditException("An edit transaction must contain at least one proposal.");
        for(int i=1;i<entries.Length;i++)
            if(!ReferenceEquals(entries[i-1].After,entries[i].Before)) throw new EditException("Transaction proposals do not form a contiguous snapshot chain.");
        return new(entries[0].Before,entries[^1].After,provenance);
    }
    private static ImmutableArray<PatternChange> Diff(Pattern before, Pattern after)
    {
        var changes = ImmutableArray.CreateBuilder<PatternChange>();
        if (before is AppPattern a && after is AppPattern b)
        {
            var old = a.Notes.ToDictionary(n=>n.Id); var next = b.Notes.ToDictionary(n=>n.Id);
            foreach (var id in old.Keys.Union(next.Keys))
            { old.TryGetValue(id,out var x); next.TryGetValue(id,out var y); if (x != y) changes.Add(new NoteChange(id,x,y)); }
        }
        else if (before is HardwarePattern h && after is HardwarePattern k)
            for (int i=0;i<h.Tracks.Length;i++)
                if (h.Tracks[i] != k.Tracks[i]) changes.Add(new TrackChange(h.Tracks[i].Id,h.Tracks[i],k.Tracks[i]));
        return changes.ToImmutable();
    }
}
/// <summary>Single-owner local transaction history. No device effects. Undo/redo restore content with a fresh revision.</summary>
public sealed class EditHistory
{
    private readonly Stack<EditProposal> undo = new();
    private readonly Stack<EditProposal> redo = new();
    public Pattern Current { get; private set; }
    public ImmutableArray<EditProposal> AcceptedHistory => undo.Reverse().ToImmutableArray();
    public bool CanUndo => undo.Count > 0;
    public bool CanRedo => redo.Count > 0;
    public EditHistory(Pattern initial) { PatternValidation.Validate(initial); Current = initial; }
    public EditHistory Fork()
    {
        var copy=new EditHistory(Current);
        foreach(var item in undo.Reverse())copy.undo.Push(item);
        foreach(var item in redo.Reverse())copy.redo.Push(item);
        return copy;
    }
    public void Apply(EditProposal proposal)
    {
        // Require the captured snapshot, not merely a caller-supplied matching revision number.
        if (!ReferenceEquals(Current,proposal.Before)) throw new EditException("Stale proposal: regenerate against the current snapshot.");
        PatternValidation.Validate(proposal.After);
        if (proposal.Changes.Length == 0) return;
        Current = proposal.After; undo.Push(proposal); redo.Clear();
    }
    public EditProposal Undo()
    {
        if (!CanUndo) throw new EditException("Nothing to undo.");
        var accepted = undo.Pop(); var inverse = new EditProposal(Current, Revision(accepted.Before),accepted.Provenance);
        Current = inverse.After; redo.Push(accepted); return inverse;
    }
    public EditProposal Redo()
    {
        if (!CanRedo) throw new EditException("Nothing to redo.");
        var accepted = redo.Pop(); var replay = new EditProposal(Current,Revision(accepted.After),accepted.Provenance);
        Current = replay.After; undo.Push(replay); return replay;
    }
    internal static Pattern Revision(Pattern p) => p switch
    {
        AppPattern a => a with { Revision = Guid.NewGuid() },
        HardwarePattern h => h with { Revision = Guid.NewGuid() },
        _ => throw new EditException("Unsupported document.")
    };
}


