using System.Collections.Immutable;
namespace Sloop.Sequencing;
public enum AppEditKind { Move, Resize, Transpose, Duplicate, Quantize, Velocity, Variation }
/// <summary>Amount is interpreted by the named operation: ticks for Move/Resize/Duplicate/Quantize; semitones for Transpose; MIDI units for Velocity; retention divisor for Variation.</summary>
public sealed record AppEdit
{
    public AppEditKind Kind { get; }
    internal long Amount { get; }
    public int Seed { get; }
    private AppEdit(AppEditKind kind,long amount,int seed=0) { Kind=kind; Amount=amount; Seed=seed; }
    public static AppEdit Move(Tick delta) => new(AppEditKind.Move,delta.Value);
    public static AppEdit Resize(Tick delta) => new(AppEditKind.Resize,delta.Value);
    public static AppEdit Duplicate(Tick offset) => new(AppEditKind.Duplicate,offset.Value);
    public static AppEdit Quantize(Tick grid) => new(AppEditKind.Quantize,grid.Value);
    public static AppEdit Transpose(int semitones) => new(AppEditKind.Transpose,semitones);
    public static AppEdit Velocity(int delta) => new(AppEditKind.Velocity,delta);
    public static AppEdit Variation(int retentionDivisor,int seed) => new(AppEditKind.Variation,retentionDivisor,seed);
}
public static class AppEditor
{
    public static EditProposal PutNote(AppPattern source, Guid id, AppNote? note, EditLocks? locks=null)
    {
        PatternValidation.Validate(source); locks??=EditLocks.None;
        var old=source.Notes.FirstOrDefault(n=>n.Id==id);
        if(id==Guid.Empty || note is not null && note.Id!=id) throw new EditException("Invalid note identity.");
        if(old==note) return new(source,source,new("Note unchanged","manual/1"));
        if(locks.Events.Contains(id) || old is not null && locks.Parts.Contains(old.PartId) || note is not null && locks.Parts.Contains(note.PartId))
            throw new EditException("Note or part is locked.");
        var notes=source.Notes.Where(n=>n.Id!=id).ToImmutableArray();
        if(note is not null) notes=notes.Add(note);
        var after=source with {Revision=Guid.NewGuid(),Notes=notes}; PatternValidation.Validate(after);
        return new(source,after,new(note is null ? "Remove note":"Edit note","manual/1"));
    }
    public static EditProposal Propose(AppPattern source, AppSelection selection, AppEdit edit, EditLocks? locks = null, string? prompt = null)
    {
        PatternValidation.Validate(source); selection.Range?.Validate(); locks ??= EditLocks.None;
        if (selection.NoteIds.Any(id=>!source.Notes.Any(n=>n.Id==id))) throw new EditException("Selection contains unknown note identities.");
        if (edit.Kind == AppEditKind.Quantize && edit.Amount <= 0) throw new EditException("Quantize grid must be positive ticks.");
        if (edit.Kind == AppEditKind.Variation && edit.Amount < 1) throw new EditException("Variation retention divisor must be positive.");
        var result = ImmutableArray.CreateBuilder<AppNote>(); int selectedOrdinal = 0;
        foreach (var note in source.Notes)
        {
            if (!selection.Includes(note)) { result.Add(note); continue; }
            var next = note;
            bool remove = false;
            switch (edit.Kind)
            {
                case AppEditKind.Move: next = note with { Start = new(checked(note.Start.Value + edit.Amount)) }; break;
                case AppEditKind.Resize: next = note with { Duration = new(checked(note.Duration.Value + edit.Amount)) }; break;
                case AppEditKind.Transpose: next = note with { Pitch = checked(note.Pitch + (int)edit.Amount) }; break;
                case AppEditKind.Velocity: next = note with { Velocity = checked(note.Velocity + (int)edit.Amount) }; break;
                case AppEditKind.Quantize:
                    long remainder = note.Start.Value % edit.Amount;
                    long lower = note.Start.Value - remainder;
                    next = note with { Start = new(remainder >= edit.Amount - remainder ? checked(lower + edit.Amount) : lower) }; break;
                case AppEditKind.Duplicate:
                    if (locks.Parts.Contains(note.PartId) || locks.Events.Contains(note.Id)) throw new EditException("Duplicate would change locked material.");
                    result.Add(note); next = note with { Id = Guid.NewGuid(), Start = new(checked(note.Start.Value + edit.Amount)) }; break;
                case AppEditKind.Variation:
                    // Deterministic thinning in stable onset/identity-independent source order, with seed as phase.
                    remove = ((long)selectedOrdinal++ + ((long)edit.Seed % edit.Amount + edit.Amount) % edit.Amount) % edit.Amount != 0; break;
                default: throw new EditException("Unsupported app edit.");
            }
            if ((next != note || remove) && (locks.Parts.Contains(note.PartId) || locks.Events.Contains(note.Id)))
                throw new EditException("Edit would change locked material.");
            if (!remove) result.Add(next);
        }
        var after = source with { Revision = Guid.NewGuid(), Notes = result.ToImmutable() };
        PatternValidation.Validate(after);
        return new(source,after,new(edit.Kind.ToString(),"sequencing-rules/1",edit.Kind==AppEditKind.Variation ? edit.Seed : null,prompt));
    }
}

