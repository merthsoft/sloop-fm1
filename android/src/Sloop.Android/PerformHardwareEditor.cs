using Android.Views;
using Android.Widget;
using Sloop.Protocol;
namespace Sloop.Android;

public sealed partial class MainActivity
{
    readonly List<HardwareGestureButton> hardwareGestureButtons = [];
    void AddHardwarePerformanceControls()
    {
        hardwareGestureButtons.Clear();
        if (!connection.CanPerformHardware) return;
        content.AddView(Label("FM1 fills & punch FX · hold to play · panel FX takes priority", 12));
        LinearLayout? row = null;
        void Add(string title, HardwareGestureKind kind, byte value, int id)
        {
            var button = new HardwareGestureButton(this) {
                Text = title,
                AccessibleHoldMilliseconds=kind==HardwareGestureKind.NextBarFill?(int)Math.Ceiling(240000.0/performOptions.Tempo)+150:350,
                ContentDescription=title+". Activate for a brief hold, then release automatically."
            };
            button.GesturePressed += () => HardwareAction(() => connection.PressHardwareAsync(id, kind, value));
            button.GestureReleased += () => HardwareAction(() => connection.ReleaseHardwareAsync(id));
            hardwareGestureButtons.Add(button);
            if (hardwareGestureButtons.Count <= 2 || (hardwareGestureButtons.Count - 3) % 4 == 0)
            {
                if (hardwareGestureButtons.Count != 2) { row = new(this) { Orientation = Orientation.Horizontal }; content.AddView(row); }
            }
            row!.AddView(button, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WrapContent, 1));
        }
        Add("Hold fill", HardwareGestureKind.Fill, 0, 1000);
        Add("Next bar fill · keep held", HardwareGestureKind.NextBarFill, 0, 1001);
        string[] effects = ["Loop 4", "Loop 8", "Loop 16", "Loop 32", "Stutter", "Reverse", "Tape stop", "Half speed", "Low pass", "High pass", "Phone", "Crush", "Alias", "Gate", "Echo", "Wobble", "Kill low", "Kill mid", "Kill high", "Rewind", "Octave up", "Flanger", "Auto-pan", "Freeze", "Loop 8T", "Mono", "Fade out"];
        for (byte i = 0; i < Math.Min(effects.Length,connection.HardwareEffectCount); i++) Add(effects[i], HardwareGestureKind.Punch, i, 1010 + i);
    }
    async void HardwareAction(Func<Task> action)
    {
        try { await action(); } catch (Exception error) { if(CanShowUi)Toast.MakeText(this, error.Message, ToastLength.Short)?.Show(); }
    }
    void ClearHardwarePerformanceTouches()
    {
        foreach (var button in hardwareGestureButtons) button.ClearTouch();
        HardwareAction(() => connection.ClearHardwarePerformanceAsync());
    }
}
sealed class HardwareGestureButton(global::Android.Content.Context context) : Button(context)
{
    int? pointer;
    bool suppressed;
    readonly UiBoundedGesture accessibleGesture=new();
    public int AccessibleHoldMilliseconds {get;init;}=350;
    public event Action? GesturePressed;
    public event Action? GestureReleased;
    public void ClearTouch() { accessibleGesture.Stop();if (pointer.HasValue) { suppressed = true; GestureReleased?.Invoke(); } }
    public override bool OnTouchEvent(MotionEvent? e)
    {
        if (e is null) return false;
        if (e.ActionMasked == MotionEventActions.Down)
        {
            accessibleGesture.Stop();
            pointer = e.GetPointerId(e.ActionIndex); suppressed = false;
            Parent?.RequestDisallowInterceptTouchEvent(true); GesturePressed?.Invoke(); return true;
        }
        if (e.ActionMasked == MotionEventActions.Cancel ||
            (e.ActionMasked is MotionEventActions.Up or MotionEventActions.PointerUp && pointer == e.GetPointerId(e.ActionIndex)))
        {
            if (pointer.HasValue && !suppressed) GestureReleased?.Invoke();
            pointer = null; suppressed = false; Parent?.RequestDisallowInterceptTouchEvent(false); base.PerformClick();
        }
        return true;
    }
    public override bool PerformClick()
    {
        if(!Enabled||!IsAttachedToWindow||pointer.HasValue)return false;
        base.PerformClick();_ = ActivateAccessibleAsync();return true;
    }
    async Task ActivateAccessibleAsync()
    {
        try {
            await accessibleGesture.ActivateAsync(()=>GesturePressed?.Invoke(),()=>GestureReleased?.Invoke(),
                ()=>Task.Delay(AccessibleHoldMilliseconds));
        }catch(Exception error){global::Android.Util.Log.Warn("MerthSloop","Accessible gesture failed: "+error.Message);}
    }
    protected override void OnDetachedFromWindow() { ClearTouch(); base.OnDetachedFromWindow(); }
}
