// Host shell for the actual production HardwareGestureButton dispatch. No Android acceptance claim.
namespace Android.Content
{
    public class Context {public const string AccessibilityService="accessibility";public object? GetSystemService(string name)=>null;}
    public class Intent(string action)
    {
        public const string ActionCreateDocument="create",CategoryOpenable="openable",ExtraTitle="title";
        public void AddCategory(string category) { }
        public void SetType(string mime) { }
        public void PutExtra(string key,string value) { }
    }
}
namespace Android.OS
{
    public class Bundle
    {
        readonly Dictionary<string,string?> fields=[];
        public void PutString(string key,string? value)=>fields[key]=value;
        public string? GetString(string key)=>fields.GetValueOrDefault(key);
    }
}
namespace Android.App
{
    public class AlertDialog
    {
        public class Builder(object context)
        {
            public Builder SetTitle(string title)=>this;
            public Builder SetMessage(string message)=>this;
            public Builder SetPositiveButton(string label,Action<object?,EventArgs> click)=>this;
            public Builder SetNegativeButton(string label,Action<object?,EventArgs> click)=>this;
            public void Show() { }
        }
    }
}
namespace Android.Views.Accessibility {public class AccessibilityManager {public bool IsTouchExplorationEnabled=>true;}}
namespace Android.Views
{
    public enum MotionEventActions {Down,Up,PointerUp,Cancel,Move}
    public class MotionEvent
    {
        public MotionEventActions ActionMasked {get;init;}
        public int ActionIndex=>0;
        public int GetPointerId(int index)=>7;
    }
    public class ViewGroup {public class LayoutParams {public const int WrapContent=-2;}}
    public class ParentView {public void RequestDisallowInterceptTouchEvent(bool value) {}}
}
namespace Android.Widget
{
    public enum Orientation {Horizontal}
    public class TextView { }
    public class Button(Android.Content.Context context):TextView
    {
        public string? Text {get;set;}
        public string? ContentDescription {get;set;}
        public event EventHandler? Click;
        public bool Enabled {get;set;}=true;
        public bool IsAttachedToWindow {get;set;}=true;
        public Android.Views.ParentView? Parent=>null;
        public virtual bool OnTouchEvent(Android.Views.MotionEvent? e)=>false;
        public virtual bool PerformClick(){Click?.Invoke(this,EventArgs.Empty);return true;}
        protected virtual void OnDetachedFromWindow() { }
    }
    public class LinearLayout(Android.Content.Context context)
    {
        public Orientation Orientation {get;init;}
        public void AddView(object view,object? layout=null) { }
        public class LayoutParams(int width,int height,int weight) { }
    }
    public enum ToastLength {Short}
    public class Toast {public static Toast MakeText(object context,string text,ToastLength length)=>new();public void Show() {}}
}
namespace Android.Util { public static class Log {public static void Warn(string tag,string text) {}} }
namespace Sloop.Protocol {public enum HardwareGestureKind {Fill,NextBarFill,Punch}}
namespace Sloop.Core {public enum Workspace {Perform,Sample}}
namespace Sloop.Android
{
    public sealed partial class MainActivity:global::Android.Content.Context
    {
        readonly FakeConnection connection=new();
        readonly FakeOptions performOptions=new();
        readonly global::Android.Widget.LinearLayout content=new(new());
        const int ExportFileRequest=415;
        public string TestRoot {get;set;}="";
        public FakeFileDir FilesDir=>new(TestRoot);
        public bool IsFinishing=>false;
        public bool IsDestroyed=>false;
        public string EffectiveWorkspaceRoot=>TestRoot+"/workspace";
        readonly FakeState state=new();
        readonly FakeSample samples=new();
        readonly FakeEditing editing=new();
        public int PickersLaunched {get;private set;}
        void StartActivityForResult(global::Android.Content.Intent intent,int request)=>PickersLaunched++;
        void Section(string key,string title,Action build,bool initiallyOpen=false)=>build();
        string[] testLabels=[];
        string[] PerformanceLabels()=>testLabels;
        public readonly List<(int Cell,bool Accessible)> PadOnsets=[];
        public List<string> PadReleases=>connection.Performer.Releases;
        void PlayPerformance(int pointer,int cell,int velocity,bool held,bool freshChord=true,bool accessible=false)=>PadOnsets.Add((cell,accessible));
        void OnPerformanceFailure(Exception error)=>throw new Exception("Unexpected performance failure",error);
        public void BuildPads(string[] labels){testLabels=labels;AddAccessiblePerformancePads(labels);}
        public global::Android.Widget.Button Pad(int cell)=>accessiblePads[cell];
        public void RefreshLabels(string[] labels){testLabels=labels;UpdateAccessiblePadLabels();}
        public void StopPads()=>accessiblePadGesture.Stop();
        public void BeginImport(int track)=>soundImportRequest=FileRequests.BeginImport(EffectiveWorkspaceRoot,track).Id;
        public int ImportTrack=>FileRequests.RequireImport(soundImportRequest!.Value,EffectiveWorkspaceRoot).Track;
        public void CompleteImport()=>FileRequests.Complete(soundImportRequest!.Value);
        public void SavePickerState(global::Android.OS.Bundle bundle)=>SaveUiRequests(bundle);
        public void RestorePickerState(global::Android.OS.Bundle bundle)=>RestoreUiRequests(bundle);
        public void Export(byte[] bytes)=>StageFileExport("test.syx","test",s=>s.Write(bytes));
        public void CopyPendingExport(Stream destination)=>FileRequests.CopyExport(fileExportRequest!.Value,destination);
        public void CompleteExport()=>FinishFileExport(fileExportRequest!.Value);
        public long BeginSampleReview()=>sampleUiRequest.Begin();
        public bool CanReviewSample(long request)=>CurrentSampleRequest(request,samples.Document,EffectiveWorkspaceRoot);
        public void SimulateStop(){activityStopped=true;sampleUiRequest.Invalidate();}
        public void SimulateDestroy(){activityDestroyed=true;sampleUiRequest.Stop();}
        global::Android.Widget.TextView Label(string text,int size)=>new();
    }
    sealed class FakeOptions {public int Tempo=>100;}
    sealed class FakeConnection
    {
        public FakePerformer Performer {get;}=new();
        public bool CanPerformHardware=>true;
        public int HardwareEffectCount=>27;
        public Task PressHardwareAsync(int id,Sloop.Protocol.HardwareGestureKind kind,byte value)=>Task.CompletedTask;
        public Task ReleaseHardwareAsync(int id)=>Task.CompletedTask;
        public Task ClearHardwarePerformanceAsync()=>Task.CompletedTask;
    }
    sealed class FakePerformer {public List<string> Releases {get;}=[];public void Release(string owner)=>Releases.Add(owner);}
    public sealed record FakeFileDir(string AbsolutePath);
    sealed class FakeState {public Sloop.Core.Workspace Workspace=>Sloop.Core.Workspace.Perform;}
    sealed class FakeSample {public object Document {get;}=new();}
    sealed class FakeEditing
    {
        public FakeSequence Sequence {get;}=new();
        public void SetStatus(string value) { }
        public void Run(Action action,string success)=>action();
    }
    sealed class FakeSequence {public object Current {get;}=new();}
}
