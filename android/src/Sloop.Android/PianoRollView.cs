using Android.Content;
using Android.Graphics;
using Android.Views;
using Sloop.Sequencing;

namespace Sloop.Android;

/// <summary>Six finger-sized pitch rows. Editing is modal; pointer cancellation never commits.</summary>
public sealed class PianoRollView : View
{
    readonly AppPattern pattern;
    readonly int channel, velocity;
    readonly long duration;
    readonly PianoRollModel model;
    readonly Action<EditProposal?> completed;
    readonly Action<string> failed;
    readonly Paint paint = new() { AntiAlias = true };
    readonly float density;
    bool multiTouch;
    public PianoRollView(Context context, AppPattern pattern, int channel, PianoRollModel model,
        int velocity, long duration, Action<EditProposal?> completed, Action<string> failed) : base(context)
    {
        this.pattern = pattern; this.channel = channel; this.model = model;
        this.velocity = velocity; this.duration = duration; this.completed = completed; this.failed = failed;
        density = context.Resources?.DisplayMetrics?.Density ?? 1;
        ContentDescription = "App piano roll. Choose Select, Draw, Move, Resize or Pan using the buttons above. Six pitch rows.";
        Focusable = true;
    }
    protected override void OnDraw(Canvas canvas)
    {
        base.OnDraw(canvas);
        float row = Height / (float)PianoRollModel.Rows;
        for (int i = 0; i < PianoRollModel.Rows; i++)
        {
            int pitch = model.TopPitch - i;
            paint.Color = new Color(pitch % 12 is 1 or 3 or 6 or 8 or 10 ? 30 : 43, 43, 55);
            canvas.DrawRect(0, i * row, Width, (i + 1) * row, paint);
        }
        paint.Color = new Color(76, 89, 105); paint.StrokeWidth = density;
        for (long tick = model.Left / model.Grid * model.Grid; tick <= model.Left + model.Span; tick += model.Grid)
        {
            float x = (tick - model.Left) * Width / (float)model.Span;
            canvas.DrawLine(x, 0, x, Height, paint);
        }
        for (int i = 0; i <= PianoRollModel.Rows; i++) canvas.DrawLine(0, i * row, Width, i * row, paint);
        foreach (var note in pattern.Notes.Where(n => n.Channel == channel))
            DrawNote(canvas, model.Previews.FirstOrDefault(n => n.Id == note.Id) ?? note, row);
        if (model.SelectionBox is { } box)
        {
            paint.Color = new Color(80, 216, 211, 70);
            canvas.DrawRect((float)Math.Min(box.X1, box.X2), (float)Math.Min(box.Y1, box.Y2), (float)Math.Max(box.X1, box.X2), (float)Math.Max(box.Y1, box.Y2), paint);
        }
        paint.Color = Color.White; paint.TextSize = 12 * density;
        for (int i = 0; i < PianoRollModel.Rows; i++) canvas.DrawText($"{model.TopPitch - i}", 4 * density, i * row + 15 * density, paint);
    }
    void DrawNote(Canvas canvas, AppNote note, float row)
    {
        int r = model.TopPitch - note.Pitch;
        if (r < 0 || r >= PianoRollModel.Rows || note.Start.Value >= model.Left + model.Span || note.Start.Value + note.Duration.Value <= model.Left) return;
        float x = (note.Start.Value - model.Left) * Width / (float)model.Span;
        float end = (note.Start.Value + note.Duration.Value - model.Left) * Width / (float)model.Span;
        paint.Color = model.Locks.Events.Contains(note.Id) || model.Locks.Parts.Contains(note.PartId) ? new Color(183, 145, 64) : model.SelectedIds.Contains(note.Id) ? new Color(80, 216, 211) : new Color(88, 143, 194);
        canvas.DrawRoundRect(Math.Max(0, x) + density, r * row + 18 * density, Math.Min(Width, end) - density, (r + 1) * row - 3 * density, 4 * density, 4 * density, paint);
    }
    public override bool OnTouchEvent(MotionEvent? e)
    {
        if (e is null) return false;
        switch (e.ActionMasked)
        {
            case MotionEventActions.Down:
                multiTouch = false;
                Parent?.RequestDisallowInterceptTouchEvent(true);
                model.Begin(pattern, channel, e.GetX(), e.GetY(), Width, Height, velocity, duration);
                return true;
            case MotionEventActions.PointerDown:
                multiTouch = true; model.Cancel(); Invalidate(); return true;
            case MotionEventActions.Move:
                if (!multiTouch) model.Update(e.GetX(), e.GetY(), 8 * density);
                Invalidate(); return true;
            case MotionEventActions.Up:
                Parent?.RequestDisallowInterceptTouchEvent(false);
                if (!multiTouch)
                {
                    try { var proposal = model.End(e.GetX(), e.GetY(), 8 * density, 48 * density); PerformClick(); completed(proposal); }
                    catch (Exception ex) { failed(ex.Message); }
                }
                model.Cancel(); Invalidate(); return true;
            case MotionEventActions.Cancel:
                model.Cancel(); Parent?.RequestDisallowInterceptTouchEvent(false); Invalidate(); return true;
            default: return true;
        }
    }
    public override bool PerformClick() { base.PerformClick(); return true; }
    protected override void OnDetachedFromWindow() { model.Cancel(); base.OnDetachedFromWindow(); }
}
