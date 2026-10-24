using Sloop.Android;
using Android.Views;

int checks=0;
void Check(bool ok,string description){if(!ok)throw new Exception(description);checks++;}
void Reject(Action action,string description){try{action();}catch(IOException){checks++;return;}throw new Exception(description);}
string directory=Path.Combine(Path.GetTempPath(),"sloop-ui-"+Guid.NewGuid().ToString("N"));
try {
    var original=new UiFileRequests(directory);
    for(int track=0;track<3;track++) {
        var pending=original.BeginImport("workspace-a",track);
        // The only saved Activity field is the ID. No reference to the old store/activity is reused.
        var restoredId=Guid.Parse(pending.Id.ToString());
        var restored=new UiFileRequests(directory);
        Check(restored.RequireImport(restoredId,"workspace-a").Track==track,"Recreation preserves the exact synth destination");
        Reject(()=>restored.RequireImport(restoredId,"workspace-b"),"Changed workspace must reject import");
        restored.Complete(restoredId);
        Reject(()=>restored.RequireImport(restoredId,"workspace-a"),"Consumed import must not default to synth one");
    }
    Reject(()=>original.RequireImport(Guid.NewGuid(),"workspace-a"),"Missing restored request must reject");
    byte[] expected=Enumerable.Range(0,8192).Select(i=>(byte)i).ToArray();
    foreach(string name in new[]{"voice.syx","backup.fm1","session.sloopzip","composition.sloopdraft"}) {
        var export=original.StageExport(name,"application/octet-stream",output=>output.Write(expected));
        var restored=new UiFileRequests(directory);
        Check(restored.RequireExport(export.Id).Name==name,"Export metadata survives recreation");
        try {using var failed=new FailedStream();restored.CopyExport(export.Id,failed);throw new Exception("Expected provider failure");}catch(IOException){checks++;}
        using var destination=new MemoryStream();new UiFileRequests(directory).CopyExport(export.Id,destination);
        Check(destination.ToArray().SequenceEqual(expected),"Provider failure retains exact bytes for retry");
        restored.Complete(export.Id);Check(!Directory.EnumerateFiles(directory).Any(),"Completed export removes metadata and bytes");
    }
    var cancelled=original.StageExport("cancel","test",s=>s.Write(expected));original.Complete(cancelled.Id);
    Check(!Directory.EnumerateFiles(directory).Any(),"Cancel cleans staged export");
    try {original.StageExport("partial","test",s=>{s.WriteByte(1);throw new IOException();});}catch(IOException){checks++;}
    Check(!Directory.EnumerateFiles(directory).Any(),"Failed staging cleans partial bytes");
    var orphan=original.StageExport("old","test",s=>s.Write(expected));
    var retained=original.BeginImport("workspace-a",2);
    foreach(var file in Directory.GetFiles(directory))File.SetLastWriteTimeUtc(file,DateTime.UtcNow.AddDays(-10));
    original.RemoveExpired(DateTime.UtcNow.AddDays(-7),retained.Id);
    Reject(()=>original.RequireExport(orphan.Id),"Abandoned staged files expire");
    Check(original.RequireImport(retained.Id,"workspace-a").Track==2,"Saved pending request survives orphan cleanup");
    original.Complete(retained.Id);

    // Actual MainActivity saved-state binding, with Android Bundle/Intent/window APIs shelled.
    string activityRoot=Path.Combine(directory,"activity");
    for(int track=0;track<3;track++) {
        var oldActivity=new MainActivity{TestRoot=activityRoot};oldActivity.BeginImport(track);
        var saved=new Android.OS.Bundle();oldActivity.SavePickerState(saved);
        var replacement=new MainActivity{TestRoot=activityRoot};replacement.RestorePickerState(saved);
        Check(replacement.ImportTrack==track,"Activity Bundle binding restores captured import destination");replacement.CompleteImport();
    }
    var exportingActivity=new MainActivity{TestRoot=activityRoot};exportingActivity.Export(expected);
    var exportState=new Android.OS.Bundle();exportingActivity.SavePickerState(exportState);
    var replacementExporter=new MainActivity{TestRoot=activityRoot};replacementExporter.RestorePickerState(exportState);
    using(var copied=new MemoryStream()) {
        replacementExporter.CopyPendingExport(copied);
        Check(copied.ToArray().SequenceEqual(expected),"Actual Activity Bundle restores staged export bytes");
    }
    replacementExporter.CompleteExport();Check(!Directory.EnumerateFiles(activityRoot).Any(),"Activity completion cleans staged files");
    var reviewingActivity=new MainActivity();long pendingReview=reviewingActivity.BeginSampleReview();
    Check(reviewingActivity.CanReviewSample(pendingReview),"Actual Activity review guard accepts a current request");
    reviewingActivity.SimulateStop();Check(!reviewingActivity.CanReviewSample(pendingReview),"Activity review binding rejects stopped activity");
    reviewingActivity=new();pendingReview=reviewingActivity.BeginSampleReview();reviewingActivity.SimulateDestroy();
    Check(!reviewingActivity.CanReviewSample(pendingReview),"Activity review binding rejects destroyed activity");

    var padActivity=new MainActivity();padActivity.BuildPads(Enumerable.Range(0,22).Select(i=>$"Chord {i}").ToArray());
    padActivity.Pad(21).PerformClick();
    Check(padActivity.PadOnsets.SequenceEqual(new[]{(21,true)}),"Native accessible expanded pad dispatches its exact cell with bounded non-latch ownership");
    padActivity.StopPads();Check(padActivity.PadReleases.SequenceEqual(new[]{"perform:-121"}),"Pad stop releases only its accessibility owner");
    await Task.Delay(400);Check(padActivity.PadReleases.Count==1,"Stopped accessible pad timeout cannot release again");
    padActivity.Pad(0).PerformClick();await Task.Delay(400);
    Check(padActivity.PadReleases.SequenceEqual(new[]{"perform:-121","perform:-100"}),"Native pad activation releases automatically");
    padActivity.RefreshLabels(Enumerable.Range(0,22).Select(i=>$"Raised chord {i}").ToArray());
    Check(padActivity.Pad(21).Text=="Raised chord 21"&&padActivity.Pad(21).ContentDescription!.Contains("Pad 22, Raised chord 21"),"Accessible labels track octave/register changes across expanded pads");

    var lifetime=new UiRequestLifetime();object source=new();long request=lifetime.Begin();
    Check(lifetime.IsCurrent(request,true,source,source,"a","a"),"Current completion may show review");
    Check(!lifetime.IsCurrent(request,false,source,source,"a","a"),"Background activity cannot show review");
    Check(!lifetime.IsCurrent(request,true,source,new object(),"a","a"),"Replaced sample/pattern cannot show review");
    Check(!lifetime.IsCurrent(request,true,source,source,"a","b"),"Changed workspace cannot show review");
    lifetime.Begin();Check(!lifetime.IsCurrent(request,true,source,source,"a","a"),"Superseded completion rejects");
    request=lifetime.Begin();lifetime.Invalidate();Check(!lifetime.IsCurrent(request,true,source,source,"a","a"),"Stop invalidates queued completion");
    request=lifetime.Begin();lifetime.Stop();Check(!lifetime.IsCurrent(request,true,source,source,"a","a"),"Destroyed Activity remains invalid even if caller reports usable");

    var bounded=new UiBoundedGesture();int on=0,off=0;
    var duration=new TaskCompletionSource();var activation=bounded.ActivateAsync(()=>on++,()=>off++,()=>duration.Task);
    Check(on==1&&off==0,"Accessible onset occurs immediately");bounded.Stop();Check(off==1,"Stop releases before timeout");
    duration.SetResult();await activation;Check(off==1,"Old timeout never releases twice");
    var a=new TaskCompletionSource();var b=new TaskCompletionSource();
    var first=bounded.ActivateAsync(()=>on++,()=>off++,()=>a.Task);
    var second=bounded.ActivateAsync(()=>on++,()=>off++,()=>b.Task);
    Check(off==2,"New activation releases previous owner");a.SetResult();await first;Check(off==2,"Superseded timeout cannot release current owner");
    b.SetResult();await second;Check(off==3,"Current owner releases on its own timeout");
    Action sharedRelease=()=>off++;a=new();b=new();
    first=bounded.ActivateAsync(()=>on++,sharedRelease,()=>a.Task);second=bounded.ActivateAsync(()=>on++,sharedRelease,()=>b.Task);
    Check(off==4,"Shared release delegate still retires previous activation");a.SetResult();await first;
    Check(off==4,"Activation identity is independent of delegate identity");b.SetResult();await second;
    Check(off==5,"Shared delegate current activation releases exactly once");
    try {await bounded.ActivateAsync(()=>throw new IOException(),()=>off++,()=>Task.CompletedTask);}catch(IOException){checks++;}
    Check(off==6,"Partial onset failure retains release obligation");

    // Actual production Button.PerformClick / OnTouchEvent, with only Android platform classes shelled.
    var hardware=new HardwareGestureButton(new());int pressed=0,released=0;
    hardware.GesturePressed+=()=>pressed++;hardware.GestureReleased+=()=>released++;
    Check(hardware.PerformClick()&&pressed==1,"Accessibility ACTION_CLICK dispatches a real hardware press");
    hardware.ClearTouch();Check(released==1,"Lifecycle clear releases accessible hardware hold immediately");
    await Task.Delay(400);Check(released==1,"Accessible hold timeout cannot duplicate cleared release");
    hardware.OnTouchEvent(new(){ActionMasked=MotionEventActions.Down});hardware.OnTouchEvent(new(){ActionMasked=MotionEventActions.Up});
    Check(pressed==2&&released==2,"Raw touch-up must not retrigger accessibility activation");
    hardware.Enabled=false;Check(!hardware.PerformClick()&&pressed==2,"Disabled accessibility control does not play");
    hardware.Enabled=true;hardware.IsAttachedToWindow=false;Check(!hardware.PerformClick()&&pressed==2,"Detached accessibility control does not play");
    var autoRelease=new HardwareGestureButton(new());var releasedAutomatically=new TaskCompletionSource();int automaticReleases=0;
    autoRelease.GestureReleased+=()=>{automaticReleases++;releasedAutomatically.TrySetResult();};
    autoRelease.PerformClick();await releasedAutomatically.Task.WaitAsync(TimeSpan.FromSeconds(2));
    Check(automaticReleases==1,"Native accessibility click releases automatically without another user action");
    var clearRaw=new HardwareGestureButton(new());int rawRelease=0;clearRaw.GestureReleased+=()=>rawRelease++;
    clearRaw.OnTouchEvent(new(){ActionMasked=MotionEventActions.Down});clearRaw.ClearTouch();
    clearRaw.OnTouchEvent(new(){ActionMasked=MotionEventActions.Up});Check(rawRelease==1,"Raw held gesture clears once, then suppresses stale touch-up");
    Console.WriteLine($"Android UI ownership: {checks} checks passed.");
} finally {if(Directory.Exists(directory))Directory.Delete(directory,true);}

sealed class FailedStream:MemoryStream {public override void Write(byte[] buffer,int offset,int count)=>throw new IOException("Provider write failure");}
