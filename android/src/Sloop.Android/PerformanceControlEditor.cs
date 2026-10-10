using Android.Graphics;
using Android.Views;
using Android.Widget;
using Sloop.Workstation;
namespace Sloop.Android;
public sealed partial class MainActivity
{
    void AddPerformanceMacro()
    {
        AddHardwarePerformanceControls();
        xySurface=null;if(!Macro.Enabled)return;
        var m=Macro;int ch=m.Track<0?PerformChannel:m.Track;
        content.AddView(Label($"XY · channel {ch+1} · X CC{m.XAxis.Controller} {m.XAxis.Minimum}–{m.XAxis.Maximum} · Y CC{m.YAxis.Controller} {m.YAxis.Minimum}–{m.YAxis.Maximum}\nRelease restores {m.XAxis.Default}/{m.YAxis.Default}. FM1 supports its mapped controllers; CC1 vibrato requires Merthsoft.10.",12));
        xySurface=new(this);xySurface.MoveMacro+=(id,x,y)=>PerformSafely(()=>{if(connection.CanPerform&&!connection.Modulation.Active)connection.Macros.Move(id,ch,m,x,y);});
        xySurface.ReleaseMacro+=id=>PerformSafely(()=>connection.Macros.Release(id));
        AddPerformanceGestureView(xySurface,120);
    }
    void AddPerformanceControlSettings(LinearLayout panel,Func<PerformOptions> get,Action<PerformOptions> set)
    {
        var mapping=get().Mapping??new();mapping.Validate();var degrees=(int[])mapping.Degrees.Clone();var directions=(ChordShape[])mapping.Directions.Clone();
        void Pick(string title,string[] values,int selected,Action<int> update){panel.AddView(Label(title,12));var v=new Spinner(this);v.Adapter=new ArrayAdapter<string>(this,global::Android.Resource.Layout.SimpleSpinnerDropDownItem,values);v.SetSelection(selected);v.ItemSelected+=(_,e)=>update(e.Position);panel.AddView(v);}
        void SaveMapping()=>set(get() with{Mapping=new(){Degrees=(int[])degrees.Clone(),Directions=(ChordShape[])directions.Clone()}});
        for(int i=0;i<7;i++){int slot=i;Pick($"Chord pad {i+1}",["I","ii","iii","IV","V","vi","vii"],degrees[i],v=>{degrees[slot]=v;SaveMapping();});}
        panel.AddView(Label("Pad 8 remains I ↑, one octave above tonic.",12));
        var shapes=Enum.GetValues<ChordShape>();string[] names=["N","NE","E","SE","S","SW","W","NW"];
        for(int i=0;i<8;i++){int slot=i;Pick("Joystick "+names[i],Enum.GetNames<ChordShape>(),(int)directions[i],v=>{directions[slot]=shapes[v];SaveMapping();});}
        var macro=get().Macro??new();void SaveMacro()=>set(get() with{Macro=macro});
        var enable=new CheckBox(this){Text="Enable XY MIDI CC macro",Checked=macro.Enabled};enable.CheckedChange+=(_,e)=>{macro=macro with{Enabled=e.IsChecked};SaveMacro();};panel.AddView(enable);
        Pick("XY destination",["Selected track / generic channel","Synth 1 / channel 1","Synth 2 / channel 2","Synth 3 / channel 3"],macro.Track+1,v=>{macro=macro with{Track=v-1};SaveMacro();});
        var allowed=Enumerable.Range(0,120).Where(c=>c is not (6 or 38 or 64 or 96 or 97 or 98 or 99 or 100 or 101)).ToArray();
        void Axis(bool x){var axis=x?macro.XAxis:macro.YAxis;string title=x?"X":"Y";void Save(){macro=x?macro with{X=axis}:macro with{Y=axis};SaveMacro();}
            Pick(title+" CC",allowed.Select(c=>c.ToString()).ToArray(),Array.IndexOf(allowed,axis.Controller),v=>{axis=axis with{Controller=allowed[v]};Save();});
            void Number(string label,int value,Action<int> update){var text=new EditText(this){Text=value.ToString(),InputType=global::Android.Text.InputTypes.ClassNumber};panel.AddView(Label(title+" "+label,12));panel.AddView(text);text.TextChanged+=(_,_)=>{if(int.TryParse(text.Text,out int n)&&n is >=0 and <=127)update(n);};}
            Number("minimum",axis.Minimum,v=>{axis=axis with{Minimum=v};Save();});Number("maximum",axis.Maximum,v=>{axis=axis with{Maximum=v};Save();});Number("release default",axis.Default,v=>{axis=axis with{Default=v};Save();});
            Pick(title+" curve",Enum.GetNames<MacroCurve>(),(int)axis.Curve,v=>{axis=axis with{Curve=(MacroCurve)v};Save();});}
        Axis(true);Axis(false);
    }
}
sealed class PerformanceXyView:View
{
    int? pointer;readonly HashSet<int> ignored=[];readonly Paint paint=new(){AntiAlias=true};readonly string caption;
    public event Action<int,double,double>? MoveMacro;public event Action<int>? ReleaseMacro;
    public PerformanceXyView(global::Android.Content.Context context,string caption="X →   Y ↑ · release resets"):base(context){this.caption=caption;Clickable=true;ContentDescription=caption;}
    public void ClearTouches(){if(pointer is {} id){ignored.Add(id);pointer=null;ReleaseMacro?.Invoke(id);}Invalidate();}
    protected override void OnDraw(Canvas c){base.OnDraw(c);paint.Color=Color.Rgb(35,51,65);c.DrawRect(0,0,Width,Height,paint);paint.Color=Color.White;paint.TextSize=28;c.DrawText(caption,12,Height/2f,paint);}
    public override bool OnTouchEvent(MotionEvent? e){if(e is null)return false;
        if(e.ActionMasked==MotionEventActions.Cancel){ClearTouches();ignored.Clear();Parent?.RequestDisallowInterceptTouchEvent(false);return true;}
        if(e.ActionMasked is MotionEventActions.Up or MotionEventActions.PointerUp){int id=e.GetPointerId(e.ActionIndex);ignored.Remove(id);if(pointer==id){pointer=null;ReleaseMacro?.Invoke(id);Parent?.RequestDisallowInterceptTouchEvent(false);}PerformClick();return true;}
        if(e.ActionMasked==MotionEventActions.Down){int id=e.GetPointerId(e.ActionIndex);ignored.Remove(id);pointer=id;Parent?.RequestDisallowInterceptTouchEvent(true);}
        if(pointer is {} active&&!ignored.Contains(active)){int i=e.FindPointerIndex(active);if(i>=0&&Width>0&&Height>0)MoveMacro?.Invoke(active,Math.Clamp(e.GetX(i)/Width,0,1),Math.Clamp(1-e.GetY(i)/Height,0,1));}return true;}
    public override bool PerformClick(){base.PerformClick();return true;}
    protected override void OnDetachedFromWindow(){ClearTouches();base.OnDetachedFromWindow();}
    protected override void Dispose(bool disposing){if(disposing)paint.Dispose();base.Dispose(disposing);}
}
