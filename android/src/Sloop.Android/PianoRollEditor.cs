using Android.Views;
using Android.Widget;
using Sloop.Sequencing;

namespace Sloop.Android;

public sealed partial class MainActivity
{
    readonly PianoRollModel pianoRoll = new();
    void ResetPianoRollForAdoptedWorkspace() => pianoRoll.ResetWorkspace();
    void AddPianoRoll(AppPattern pattern, int channel)
    {
        pianoRoll.Reconcile(pattern, channel);
        content.AddView(Label("App piano roll", 18));
        content.AddView(Label("Select one note, or use MultiSelect to toggle notes and drag a box to add notes. Move and Resize drag the whole selection. Draw adds on a tap; Pan drags the viewport. Two fingers cancel.", 14));
        var tools = new LinearLayout(this) { Orientation = Orientation.Horizontal };
        int toolIndex = 0;
        foreach (var tool in Enum.GetValues<PianoRollTool>())
        {
            if (toolIndex++ == 3) { content.AddView(tools); tools = new LinearLayout(this) { Orientation = Orientation.Horizontal }; }
            var button = new Button(this) { Text = tool.ToString(), Activated = pianoRoll.Tool == tool };
            if (tool == PianoRollTool.MultiSelect) button.Text = "Multi select";
            button.SetMinHeight(Dp(48));
            button.Click += (_, _) => { pianoRoll.Cancel(); pianoRoll.Tool = tool; ShowWorkspace(); };
            tools.AddView(button, new LinearLayout.LayoutParams(0, Dp(56), 1));
        }
        content.AddView(tools);
        ActionButton($"Snap grid · 1/{pattern.TicksPerQuarter * 4 / pianoRoll.Grid}", () =>
            new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Piano roll quantization")!
                .SetItems(new[] { "Quarter", "Eighth", "Sixteenth", "Thirty-second" }, (_, a) =>
                { pianoRoll.Grid = Math.Max(1, pattern.TicksPerQuarter / (1 << a.Which)); ShowWorkspace(); })!.Show());
        content.AddView(Label($"Ticks {pianoRoll.Left}–{pianoRoll.Left + pianoRoll.Span} · pitches {pianoRoll.TopPitch - PianoRollModel.Rows + 1}–{pianoRoll.TopPitch}", 14));
        content.AddView(new PianoRollView(this, pattern, channel, pianoRoll, editing.Velocity, editing.Duration,
            proposal => { if (proposal is null) ShowWorkspace(); else editing.Run(() => editing.EditPattern(proposal), "Piano roll edit saved. Undo is available."); },
            message => editing.SetStatus(message)), new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MatchParent, Dp(48 * PianoRollModel.Rows)));
        var selected = pattern.Notes.FirstOrDefault(n => n.Id == pianoRoll.Selected);
        content.AddView(Label(selected is null ? "No notes selected" : pianoRoll.SelectedIds.Count > 1 ? $"{pianoRoll.SelectedIds.Count} notes selected" : $"Selected note {selected.Pitch} · tick {selected.Start.Value} · duration {selected.Duration.Value} · velocity {selected.Velocity}", 14));
        RollButtons(("Select channel", () => pianoRoll.SelectAll(pattern, channel)), ("Clear selection", () => pianoRoll.ClearSelection()));
        RollButtons(("Zoom in", () => pianoRoll.Zoom(.5, pattern.Length.Value)), ("Zoom out", () => pianoRoll.Zoom(2, pattern.Length.Value)));
        RollButtons(("Earlier", () => pianoRoll.Navigate(-pianoRoll.Span / 2, 0, pattern.Length.Value)), ("Later", () => pianoRoll.Navigate(pianoRoll.Span / 2, 0, pattern.Length.Value)));
        RollButtons(("Lower pitches", () => pianoRoll.Navigate(0, -PianoRollModel.Rows, pattern.Length.Value)), ("Higher pitches", () => pianoRoll.Navigate(0, PianoRollModel.Rows, pattern.Length.Value)));
        ActionButton("Quantize selection", () => editing.EditPattern(pianoRoll.Quantize(pattern)), selected is not null);
        ActionButton("Delete selection", () => editing.EditPattern(pianoRoll.Delete(pattern)), selected is not null);
        bool allProtected = selected is not null && pianoRoll.SelectedIds.All(pianoRoll.Locks.Events.Contains);
        ActionButton(allProtected ? "Unlock selection" : "Protect selection", () =>
        { pianoRoll.ProtectSelected(!allProtected); }, selected is not null);
        RollButtons(("Undo", () => editing.PatternHistory(false)), ("Redo", () => editing.PatternHistory(true)), editing.Sequence.CanUndo, editing.Sequence.CanRedo, true);
    }
    void RollButtons((string Title, Action Action) first, (string Title, Action Action) second, bool firstEnabled = true, bool secondEnabled = true, bool history = false)
    {
        var row = new LinearLayout(this) { Orientation = Orientation.Horizontal };
        void Add((string Title, Action Action) item, bool enabled)
        {
            var button = new Button(this) { Text = item.Title, Enabled = enabled };
            button.Click += (_, _) => { if (history) editing.Run(item.Action, "Sequence history restored."); else { item.Action(); ShowWorkspace(); } };
            row.AddView(button, new LinearLayout.LayoutParams(0, Dp(52), 1));
        }
        Add(first, firstEnabled); Add(second, secondEnabled); content.AddView(row);
    }
}
