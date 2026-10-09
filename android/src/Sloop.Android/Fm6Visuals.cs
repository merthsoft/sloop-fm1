using Android.Content;
using Android.Graphics;
using Android.Views;
using Sloop.SoundDesign;
using CanvasPath = Android.Graphics.Path;

namespace Sloop.Android;

internal sealed class AlgorithmDiagram : View
{
    readonly AlgorithmGraph graph;
    readonly int selected;
    readonly Action<int> choose;
    readonly Dictionary<int, PointF> centers = [];
    readonly Paint paint = new(PaintFlags.AntiAlias);
    readonly float density;
    static readonly Color Teal = Color.Rgb(82, 220, 188);
    public AlgorithmDiagram(Context context, int algorithm, int selected, Action<int> choose) : base(context)
    {
        graph = FirmwareAlgorithms.Get(algorithm); this.selected = selected; this.choose = choose;
        density = Resources!.DisplayMetrics!.Density;
        ContentDescription = $"Algorithm {algorithm}. Carriers: {string.Join(", ", graph.Carriers.Order())}. Use operator buttons below to select.";
    }
    protected override void OnDraw(Canvas canvas)
    {
        base.OnDraw(canvas);
        centers.Clear();
        int depth = Enumerable.Range(1, 6).Max(graph.Depth);
        float radius = 17 * density, top = 25 * density, bottom = Height - 45 * density;
        for (int row = 0; row <= depth; row++)
        {
            var numbers = Enumerable.Range(1, 6).Where(n => graph.Depth(n) == row).Reverse().ToArray();
            for (int i = 0; i < numbers.Length; i++)
                centers[numbers[i]] = new(Width * (i + 1f) / (numbers.Length + 1),
                    depth == 0 ? (top + bottom) / 2 : bottom - row * (bottom - top) / depth);
        }
        paint.StrokeWidth = 2 * density; paint.Color = Color.Rgb(132, 150, 165);
        foreach (var edge in graph.Edges)
        {
            var a = centers[edge.Source]; var b = centers[edge.Target];
            canvas.DrawLine(a.X, a.Y + radius, b.X, b.Y - radius, paint);
            canvas.DrawLine(b.X, b.Y - radius, b.X - 3*density, b.Y - radius - 6*density, paint);
            canvas.DrawLine(b.X, b.Y - radius, b.X + 3*density, b.Y - radius - 6*density, paint);
        }
        float audioY = Height - 22 * density;
        paint.Color = Teal;
        foreach (int number in graph.Carriers)
        {
            var point = centers[number];
            canvas.DrawLine(point.X, point.Y + radius, point.X, audioY, paint);
        }
        canvas.DrawLine(Width * .08f, audioY, Width * .92f, audioY, paint);
        paint.TextSize = 10 * density; paint.TextAlign = Paint.Align.Center;
        canvas.DrawText("AUDIO", Width / 2f, Height - 5 * density, paint);
        foreach (int number in graph.FeedbackOperators)
        {
            var point = centers[number];
            paint.SetStyle(Paint.Style.Stroke); paint.Color = Color.Rgb(243, 184, 101);
            canvas.DrawOval(point.X - radius*1.5f, point.Y - radius*1.25f,
                point.X, point.Y + radius*1.25f, paint);
        }
        foreach (var (number, point) in centers)
        {
            paint.SetStyle(Paint.Style.Fill);
            paint.Color = number == selected ? Teal : Color.Rgb(45, 57, 69);
            canvas.DrawCircle(point.X, point.Y, radius, paint);
            paint.SetStyle(Paint.Style.Stroke);
            paint.Color = graph.Carriers.Contains(number) ? Teal : Color.Rgb(155, 169, 181);
            canvas.DrawCircle(point.X, point.Y, radius, paint);
            paint.SetStyle(Paint.Style.Fill); paint.TextSize = 14*density;
            paint.Color = number == selected ? Color.Rgb(15, 35, 31) : Color.White;
            canvas.DrawText(number.ToString(), point.X, point.Y + 5*density, paint);
        }
    }
    public override bool OnTouchEvent(MotionEvent? e)
    {
        if (e?.ActionMasked == MotionEventActions.Up)
        {
            var hit = centers.FirstOrDefault(p => Math.Abs(p.Value.X-e.GetX()) <= 23*density && Math.Abs(p.Value.Y-e.GetY()) <= 23*density);
            if (hit.Key != 0) { PerformClick(); choose(hit.Key); }
        }
        return true;
    }
    public override bool PerformClick() { base.PerformClick(); return true; }
}

internal sealed class OperatorEnvelopeView : View
{
    readonly Paint paint = new(PaintFlags.AntiAlias);
    readonly int[] rates, original;
    readonly int[] levels;
    readonly Action<int, int> commit;
    readonly float density;
    int active = -1;
    float PlotTop => 19*density;
    float PlotBottom => Height - 43*density;
    float StageX(int stage) => 20*density + stage*(Width-40*density)/4;
    float LevelY(int level) => PlotBottom - level*(PlotBottom-PlotTop)/99;
    public OperatorEnvelopeView(Context context, Envelope env, Action<int,int> commit) : base(context)
    {
        levels = [env.Level1, env.Level2, env.Level3, env.Level4]; original = (int[])levels.Clone();
        rates = [env.Rate1,env.Rate2,env.Rate3,env.Rate4]; this.commit = commit;
        density = Resources!.DisplayMetrics!.Density;
        ContentDescription = "Operator envelope. Drag a level point vertically, or use numeric controls below. Stage spacing is schematic, not time.";
    }
    protected override void OnDraw(Canvas canvas)
    {
        base.OnDraw(canvas);
        paint.SetStyle(Paint.Style.Stroke); paint.StrokeWidth = density;
        paint.Color = Color.Rgb(60, 73, 85);
        for (int i = 0; i < 3; i++) canvas.DrawLine(StageX(0), LevelY(i*49), StageX(4), LevelY(i*49), paint);
        using var path = new CanvasPath(); path.MoveTo(StageX(0), LevelY(levels[3]));
        for (int i = 0; i < 4; i++) path.LineTo(StageX(i+1),LevelY(levels[i]));
        paint.Color = Color.Rgb(82,220,188); paint.StrokeWidth = 2*density; canvas.DrawPath(path,paint);
        paint.SetStyle(Paint.Style.Fill); paint.TextAlign = Paint.Align.Center;
        for (int i = 0; i < 4; i++)
        {
            canvas.DrawCircle(StageX(i+1),LevelY(levels[i]),(active==i?8:6)*density,paint);
            paint.TextSize = 11*density;
            canvas.DrawText(levels[i].ToString(),StageX(i+1),LevelY(levels[i])-9*density,paint);
            canvas.DrawText($"L{i+1}", StageX(i+1),Height-25*density,paint);
            canvas.DrawText($"R{rates[i]}",StageX(i+1),Height-9*density,paint);
        }
        paint.Color = Color.Rgb(155,169,181); paint.TextSize = 10*density;
        canvas.DrawText("L4",StageX(0),Height-25*density,paint);
    }
    public override bool OnTouchEvent(MotionEvent? e)
    {
        if (e is null) return false;
        if (e.ActionMasked == MotionEventActions.Down)
        {
            active = Enumerable.Range(0,4).FirstOrDefault(i => Math.Abs(e.GetX()-StageX(i+1))<=24*density && Math.Abs(e.GetY()-LevelY(levels[i]))<=26*density,-1);
            if (active < 0) return false;
            Parent?.RequestDisallowInterceptTouchEvent(true);
        }
        if (active < 0) return false;
        if (e.ActionMasked is MotionEventActions.Down or MotionEventActions.Move or MotionEventActions.Up)
        {
            levels[active] = Math.Clamp((int)Math.Round((PlotBottom-e.GetY())*99/(PlotBottom-PlotTop)),0,99);
            Invalidate();
        }
        if (e.ActionMasked == MotionEventActions.Up)
        {
            int stage=active, value=levels[stage]; active=-1;
            Parent?.RequestDisallowInterceptTouchEvent(false); PerformClick();
            if (value!=original[stage]) commit(stage+1,value);
        }
        else if (e.ActionMasked == MotionEventActions.Cancel)
        {
            original.CopyTo(levels,0); active=-1; Invalidate();
            Parent?.RequestDisallowInterceptTouchEvent(false);
        }
        return true;
    }
    public override bool PerformClick() { base.PerformClick(); return true; }
}
