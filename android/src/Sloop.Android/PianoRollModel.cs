using System.Collections.Immutable;
using Sloop.Sequencing;

namespace Sloop.Android;

public enum PianoRollTool { Select, Draw, Move, Resize, Pan, MultiSelect }

/// <summary>Platform-free touch state. A gesture captures its source; only release produces an edit.</summary>
public sealed class PianoRollModel
{
    public PianoRollTool Tool { get; set; }
    ImmutableHashSet<Guid> selectedIds = [];
    public ImmutableHashSet<Guid> SelectedIds => selectedIds;
    public Guid? Selected { get => selectedIds.Count == 0 ? null : selectedIds.Order().First(); set => selectedIds = value is Guid id ? [id] : []; }
    public void ClearSelection() => selectedIds = [];
    public void SelectAll(AppPattern pattern, int editChannel) => selectedIds = pattern.Notes.Where(n => n.Channel == editChannel).Select(n => n.Id).ToImmutableHashSet();
    public long Grid { get; set; } = 240;
    public long Left { get; private set; }
    public long Span { get; private set; } = 1920;
    public int TopPitch { get; private set; } = 65;
    public const int Rows = 6;
    public EditLocks Locks { get; private set; } = EditLocks.None;
    /// <summary>Host hook for shared part/event protection; proposals always use these exact locks.</summary>
    public void SetLocks(EditLocks locks) => Locks = locks;
    public AppNote? Preview { get; private set; }
    public ImmutableArray<AppNote> Previews { get; private set; } = [];
    public (double X1, double Y1, double X2, double Y2)? SelectionBox { get; private set; }
    AppPattern? source;
    ImmutableArray<AppNote> originals = [];
    double downX, downY, width, height;
    long initialLeft;
    int initialTop;
    bool traveled;
    int channel, velocity;
    long duration;

    public void ProtectSelected(bool protect)
    {
        Locks = Locks with { Events = protect ? Locks.Events.Union(selectedIds) : Locks.Events.Except(selectedIds) };
    }
    public void Reconcile(AppPattern pattern, int editChannel)
    {
        // A release from the previous view must not edit its old channel or source.
        if (source is not null && (!ReferenceEquals(source, pattern) || channel != editChannel)) Cancel();
        selectedIds = selectedIds.Intersect(pattern.Notes.Where(n => n.Channel == editChannel).Select(n => n.Id));
        // Missing IDs can return through Undo/Redo. Protection belongs to this editing session.
        ClampViewport(pattern.Length.Value);
    }
    public void ResetWorkspace()
    {
        Cancel(); Selected = null; Locks = EditLocks.None; Left = 0; Span = 1920; TopPitch = 65;
    }
    public void Zoom(double factor, long length)
    {
        long center = Left + Span / 2;
        Span = Math.Clamp((long)(Span * factor), Math.Min(480, length), length);
        Left = center - Span / 2;
        ClampViewport(length);
    }
    public void Navigate(long ticks, int pitches, long length)
    {
        Left += ticks; TopPitch += pitches; ClampViewport(length);
    }
    void ClampViewport(long length)
    {
        Span = Math.Min(Span, length);
        Left = Math.Clamp(Left, 0, Math.Max(0, length - Span));
        TopPitch = Math.Clamp(TopPitch, Rows - 1, 127);
    }
    public long Snap(long ticks) => checked((long)Math.Floor((double)ticks / Grid + .5) * Grid);
    public long TickAt(double x, double w) => Left + (long)(Math.Clamp(x / w, 0, 1) * Span);
    public int PitchAt(double y, double h) => TopPitch - Math.Clamp((int)(y / (h / Rows)), 0, Rows - 1);

    public Guid? Hit(AppPattern pattern, int editChannel, double x, double y, double w, double h, double minimumWidth)
    {
        int pitch = PitchAt(y, h);
        // Prefer a literal rectangle, then nearest center within a finger-sized target.
        return pattern.Notes.Where(n => n.Channel == editChannel && n.Pitch == pitch)
            .Where(n => n.Start.Value < Left + Span && n.Start.Value + n.Duration.Value > Left)
            .Select(n => new { Note = n, Start = (n.Start.Value - Left) * w / Span, End = (n.Start.Value + n.Duration.Value - Left) * w / Span })
            .Where(n => x >= n.Start - Math.Max(0, minimumWidth - (n.End - n.Start)) / 2 && x <= n.End + Math.Max(0, minimumWidth - (n.End - n.Start)) / 2)
            .OrderBy(n => x >= n.Start && x <= n.End ? 0 : 1)
            .ThenBy(n => Math.Abs(x - (n.Start + n.End) / 2))
            .Select(n => (Guid?)n.Note.Id).FirstOrDefault();
    }
    public void Begin(AppPattern pattern, int editChannel, double x, double y, double w, double h, int noteVelocity, long noteDuration)
    {
        Cancel();
        if (!double.IsFinite(w) || !double.IsFinite(h) || w <= 0 || h <= 0 ||
            !double.IsFinite(x) || !double.IsFinite(y) || x < 0 || x > w || y < 0 || y > h) return;
        source = pattern; channel = editChannel; width = w; height = h;
        downX = x; downY = y; initialLeft = Left; initialTop = TopPitch;
        velocity = noteVelocity; duration = noteDuration;
        originals = pattern.Notes.Where(n => selectedIds.Contains(n.Id) && n.Channel == channel).ToImmutableArray();
    }
    public void Update(double x, double y, double touchSlop)
    {
        if (source is null) return;
        if (!double.IsFinite(x) || !double.IsFinite(y)) { Cancel(); return; }
        traveled |= Math.Abs(x - downX) > touchSlop || Math.Abs(y - downY) > touchSlop;
        if (Tool == PianoRollTool.Pan)
        {
            Left = initialLeft - (long)((x - downX) * Span / width);
            TopPitch = initialTop + (int)Math.Round((y - downY) * Rows / height);
            ClampViewport(source.Length.Value); return;
        }
        if (Tool == PianoRollTool.MultiSelect && traveled) { SelectionBox = (downX, downY, Math.Clamp(x, 0, width), Math.Clamp(y, 0, height)); return; }
        if (!traveled || originals.IsEmpty) return;
        long delta = Snap((long)((x - downX) * Span / width));
        if (Tool == PianoRollTool.Move)
        {
            delta = Math.Clamp(delta, -originals.Min(n => n.Start.Value), originals.Min(n => source.Length.Value - n.Start.Value - n.Duration.Value));
            int pitchDelta = Math.Clamp(-(int)Math.Round((y - downY) * Rows / height), -originals.Min(n => n.Pitch), 127 - originals.Max(n => n.Pitch));
            Previews = originals.Select(n => n with { Start = new(n.Start.Value + delta), Pitch = n.Pitch + pitchDelta }).ToImmutableArray();
        }
        if (Tool == PianoRollTool.Resize)
        {
            delta = Math.Clamp(delta, originals.Max(n => Math.Min(Grid, n.Duration.Value) - n.Duration.Value), originals.Min(n => source.Length.Value - n.Start.Value - n.Duration.Value));
            Previews = originals.Select(n => n with { Duration = new(n.Duration.Value + delta) }).ToImmutableArray();
        }
        Preview = Previews.FirstOrDefault(n => n.Id == Selected);
    }
    public EditProposal? End(double x, double y, double touchSlop, double minimumTarget)
    {
        if (source is null) return null;
        Update(x, y, touchSlop);
        var captured = source;
        try
        {
            if (!double.IsFinite(x) || !double.IsFinite(y)) return null;
            if (Tool is PianoRollTool.Select or PianoRollTool.Draw && (x < 0 || x > width || y < 0 || y > height)) return null;
            if (Tool == PianoRollTool.Select && !traveled)
                Selected = Hit(captured, channel, x, y, width, height, minimumTarget);
            if (Tool == PianoRollTool.MultiSelect)
            {
                if (!traveled)
                {
                    if (x < 0 || x > width || y < 0 || y > height) return null;
                    var hit = Hit(captured, channel, x, y, width, height, minimumTarget);
                    if (hit is Guid id) selectedIds = selectedIds.Contains(id) ? selectedIds.Remove(id) : selectedIds.Add(id);
                }
                else if (SelectionBox is { } box)
                {
                    long left = TickAt(Math.Min(box.X1, box.X2), width), right = TickAt(Math.Max(box.X1, box.X2), width);
                    int low = PitchAt(Math.Max(box.Y1, box.Y2), height), high = PitchAt(Math.Min(box.Y1, box.Y2), height);
                    selectedIds = selectedIds.Union(captured.Notes.Where(n => n.Channel == channel && n.Pitch >= low && n.Pitch <= high && n.Start.Value < right && n.Start.Value + n.Duration.Value > left).Select(n => n.Id));
                }
            }
            if (Tool == PianoRollTool.Draw && !traveled)
            {
                var hit = Hit(captured, channel, x, y, width, height, minimumTarget);
                if (hit is not null) { Selected = hit; return null; }
                long start = Math.Clamp(Snap(TickAt(x, width)), 0, Math.Max(0, captured.Length.Value - Grid));
                var id = Guid.NewGuid();
                var note = new AppNote(id, captured.Id, new(start), new(Math.Min(Math.Max(Grid, duration), captured.Length.Value - start)), PitchAt(y, height), velocity, channel);
                var proposal = AppEditor.PutNote(captured, id, note, Locks);
                Selected = id; return proposal;
            }
            if (!Previews.IsEmpty && !Previews.SequenceEqual(originals))
                return PutGroup(captured, Previews.Select(n => (n.Id, (AppNote?)n)));
            return null;
        }
        finally { Cancel(); }
    }
    EditProposal PutGroup(AppPattern pattern, IEnumerable<(Guid Id, AppNote? Note)> edits)
    {
        var chain = new List<EditProposal>();
        foreach (var (id, note) in edits) { var proposal = AppEditor.PutNote(pattern, id, note, Locks); chain.Add(proposal); pattern = (AppPattern)proposal.After; }
        return EditProposal.Compose(chain, new("Piano roll group edit", "piano-roll/2"));
    }
    AppSelection Selection() => new(selectedIds.IsEmpty ? throw new EditException("Select a note first.") : selectedIds);
    public EditProposal Delete(AppPattern pattern) { Selection(); return PutGroup(pattern, selectedIds.Select(id => (id, (AppNote?)null))); }
    public EditProposal Quantize(AppPattern pattern) => AppEditor.Propose(pattern,
        Selection(), AppEdit.Quantize(new(Grid)), Locks);
    public void Cancel() { source = null; originals = []; Preview = null; Previews = []; SelectionBox = null; traveled = false; }
}
