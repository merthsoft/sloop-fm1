using Android.Content;
using Android.Graphics;
using Android.Views;
using Android.Widget;
using Sloop.Android.Services;
using Sloop.Core.Sampling;
using Sloop.Workstation;

namespace Sloop.Android;

public sealed partial class MainActivity
{
    private const int ImportWaveRequest = 410, ExportWaveRequest = 411, RecordingPermissionRequest = 412;
    private bool recordingForeground;
    private TextView? sampleStatusLabel;
    private ProgressBar? captureMeter;
    private bool captureUiActive;
    private SampleWorkspace samples = null!;
    private int samplePage, selectedSampleSlice;
    private int equalSliceCount = 4;
    private SampleDocument? viewportDocument;
    private SampleViewport? sampleViewport;
    private double transientSensitivity = .5;
    private int transientSpacing = 80;
    private int tapLatencyMilliseconds;
    private void BrowseRetainedSamples() => BrowseCrossSessionSamples();
    private void OnSamplesChanged()
    {
        ObserveSceneWorkspace();
        if (samples.RecordingActive) Window?.AddFlags(WindowManagerFlags.KeepScreenOn);
        else Window?.ClearFlags(WindowManagerFlags.KeepScreenOn);
        if (state.Workspace != Sloop.Core.Workspace.Sample) return;
        if (samples.RecordingActive && captureUiActive && sampleStatusLabel is not null) {
            sampleStatusLabel.Text = samples.Status;
            if (captureMeter is not null) captureMeter.Progress = samples.CapturePeak;
        } else ShowWorkspace();
    }
    private void ChooseRecordingInput()
    {
        var inputs = samples.Inputs();
        if (inputs.Length == 0) {
            new global::Android.App.AlertDialog.Builder(this)!.SetTitle("No recording input")!.SetMessage("Connect a USB audio input or use a phone with a microphone.")!.SetPositiveButton("OK", (_, _) => { })!.Show(); return;
        }
        new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Record from - maximum 2 minutes")!
            .SetItems(inputs.Select(d => $"{d.ProductName} - {d.Type}").ToArray(), (_, args) => {
                samples.PendingCaptureInput = inputs[args.Which].Id;
                if (CheckSelfPermission(global::Android.Manifest.Permission.RecordAudio) != global::Android.Content.PM.Permission.Granted)
                    RequestPermissions([global::Android.Manifest.Permission.RecordAudio], RecordingPermissionRequest);
                else StartPendingRecording();
            })!.SetNegativeButton("Cancel", (_, _) => { })!.Show();
    }
    private void StartPendingRecording()
    {
        var input = samples.PendingCaptureInput; samples.PendingCaptureInput = null;
        if (input is not null && recordingForeground) _ = samples.RecordAsync(input.Value);
    }
    public override void OnRequestPermissionsResult(int requestCode, string[] permissions, global::Android.Content.PM.Permission[] grantResults)
    {
        base.OnRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode != RecordingPermissionRequest) return;
        if (grantResults.Length > 0 && grantResults[0] == global::Android.Content.PM.Permission.Granted) StartPendingRecording();
        else samples.CapturePermissionDenied();
    }
    private void ImportWave()
    {
        var intent = new Intent(Intent.ActionOpenDocument); intent.AddCategory(Intent.CategoryOpenable); intent.SetType("audio/*");
        StartActivityForResult(intent, ImportWaveRequest);
    }
    private void ChoosePreviewOutput()
    {
        var outputs = samples.Outputs();
        var names = new[] { "System default" }.Concat(outputs.Select(d => $"{d.ProductName} - {d.Type}")).ToArray();
        new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Preview audio output")!
            .SetItems(names, (_, args) => samples.SelectOutput(args.Which == 0 ? null : outputs[args.Which - 1]))!.SetNegativeButton("Cancel", (_, _) => { })!.Show();
    }
    private void ExportWave(PcmWave source, long start, long end, string name)
    {
        samples.PendingExport = (source, start, end);
        var intent = new Intent(Intent.ActionCreateDocument); intent.AddCategory(Intent.CategoryOpenable);
        intent.SetType("audio/wav"); intent.PutExtra(Intent.ExtraTitle, name); StartActivityForResult(intent, ExportWaveRequest);
    }
#pragma warning disable CS0672
    protected override async void OnActivityResult(int requestCode, Result resultCode, Intent? data)
    {
        base.OnActivityResult(requestCode, resultCode, data);
        await FileResultAsync(requestCode, resultCode, data);
        if (requestCode == ExportKitRequest) {
            if (resultCode == Result.Ok && data?.Data is {} kitUri) await samples.ExportKitAsync(kitUri); else samples.PendingKitExport = null;
        }
        if (requestCode == ImportWaveRequest && resultCode == Result.Ok && data?.Data is {} uri) await samples.ImportAsync(uri);
        if (requestCode == ExportWaveRequest) {
            if (resultCode == Result.Ok && data?.Data is {} destination) await samples.ExportAsync(destination); else samples.PendingExport = null;
        }
    }
#pragma warning restore CS0672
    private void AddSampleEditor()
    {
        void Button(string title, Action action, bool enabled = true) {
            var button = new global::Android.Widget.Button(this) { Text = title, Enabled = enabled && !samples.Busy };
            button.Click += (_, _) => action(); content.AddView(button);
        }
        sampleStatusLabel = Label(samples.Status, 13);
        if (connection.Snapshot.Busy) {
            kitTransferLabel = Label(string.IsNullOrEmpty(kitTransferStatus) ? connection.Snapshot.Status : kitTransferStatus, 16); content.AddView(kitTransferLabel);
            kitTransferProgress = new ProgressBar(this, null, global::Android.Resource.Attribute.ProgressBarStyleHorizontal) { Max = 100, Progress = kitTransferPercent, ContentDescription = "FM1 transfer progress" }; content.AddView(kitTransferProgress);
            content.AddView(Label("Keep the USB cable connected until verification finishes.", 14)); return;
        }
        captureUiActive = samples.RecordingActive; captureMeter = null;
        if (captureUiActive) {
            content.AddView(sampleStatusLabel);
            captureMeter = new ProgressBar(this, null, global::Android.Resource.Attribute.ProgressBarStyleHorizontal) { Max = 32768, Progress = samples.CapturePeak, ContentDescription = "Recording peak level" }; content.AddView(captureMeter);
            var stop = new global::Android.Widget.Button(this) { Text = "Stop recording - keep take" }; stop.Click += (_, _) => samples.StopRecording(); content.AddView(stop);
            content.AddView(Label("No software monitoring. Recording stops and saves when the app leaves the foreground.", 14)); return;
        }
        ButtonRow(("Record", ChooseRecordingInput, !samples.Busy), ("Import WAV", ImportWave, !samples.Busy));
        Button("Browse retained samples", BrowseRetainedSamples);
        if (samples.Document is not {} doc) { content.AddView(sampleStatusLabel); content.AddView(Label("Capture or import a PCM16 WAV to chop and build a kit.", 16)); return; }
        var source = doc.Source;
        content.AddView(Label($"{source.Frames/(double)source.SampleRate:0.00}s - {doc.Slices().Length} chops - {source.SampleRate/1000.0:0.#} kHz", 14));
        var pages = new LinearLayout(this) { Orientation = Orientation.Horizontal };
        foreach (var (title, index) in new[] { ("Chop", 0), ("Send to FM1", 1) }) {
            var tab = new global::Android.Widget.Button(this) { Text = title }; tab.Click += (_, _) => { samplePage = index; ShowWorkspace(); };
            pages.AddView(tab, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WrapContent, 1));
        }
        content.AddView(pages);
        pages.Post(() => { for (int i = 0; i < pages.ChildCount; i++) StyleButton((global::Android.Widget.Button)pages.GetChildAt(i)!, i == samplePage); });
        Button($"Listen through - {samples.OutputName}", ChoosePreviewOutput);
        if (samplePage == 1) { AddSampleKitReview(); content.AddView(sampleStatusLabel); AddKitEditor(); return; }
        if (!ReferenceEquals(viewportDocument, doc)) { viewportDocument = doc; sampleViewport = new(source.Frames); }
        var viewport = sampleViewport!;
        void Navigate(Action action) { action(); ShowWorkspace(); }
        selectedSampleSlice = Math.Clamp(selectedSampleSlice, 0, doc.Slices().Length-1);
        var waveform = new SampleWaveView(this, doc, samples.Peaks, samples.Cursor, viewport) {
            Enabled = !samples.Busy, SelectedSlice = selectedSampleSlice, ProposedBoundaries = samples.TapReview?.Boundaries ?? samples.ChopProposal?.Boundaries ?? []
        };
        content.AddView(waveform, new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MatchParent, Dp(180)));
        ButtonRow(("Split here", () => samples.Edit(d => { if (!d.Split(samples.Cursor)) throw new ArgumentException("Choose a new position inside trim; maximum 16 slices."); }), !samples.Busy),
            ("Undo", () => samples.Edit(d => d.Undo()), doc.CanUndo && !samples.Busy),
            ("Play", () => _ = samples.PreviewAsync(doc.Start, doc.End), !samples.Busy), ("Stop", samples.StopPreview, true));
        var currentSlice = doc.Slices()[selectedSampleSlice];
        Section("sample.tap", "Tap along & review", () => {
            content.AddView(Label("Play trim, tap while listening, then review orange markers. Apply replaces saved chops in one undo step.", 13));
            var latency = new Spinner(this);
            int[] delays = [0, 25, 50, 100, 150, 200, 300, 500];
            latency.Adapter = new ArrayAdapter<string>(this, global::Android.Resource.Layout.SimpleSpinnerDropDownItem,
                delays.Select(ms => $"Subtract output latency · {ms} ms").ToArray());
            latency.SetSelection(Array.IndexOf(delays, tapLatencyMilliseconds));
            latency.ItemSelected += (_, a) => tapLatencyMilliseconds = delays[a.Position]; content.AddView(latency);
            content.AddView(Label("Latency is a manual correction; Android does not report measured end-to-end output delay here.", 12));
            ButtonRow(("Start fresh tap pass", () => _ = samples.StartTapChoppingAsync(), !samples.Busy),
                ("Tap marker", () => samples.TapPlayback(tapLatencyMilliseconds), samples.CanTapPlayback));
            if (samples.TapReview is { } taps) {
                content.AddView(Label($"{taps.Boundaries.Count} proposed tap markers · " + string.Join(", ", taps.Boundaries.Select(f => $"{f / (double)source.SampleRate:0.000}s")), 13));
                ButtonRow(("Undo last tap", samples.UndoTap, taps.Boundaries.Count > 0), ("Stop & review", samples.StopPreview, true));
                ButtonRow(("Apply taps - replace chops", samples.ApplyTapReview, taps.Boundaries.Count > 0), ("Discard taps", samples.DiscardTapReview, true));
            }
        });
        AddChopAudioEditor();
        content.AddView(Label($"Selected chop {selectedSampleSlice+1:00} · {(currentSlice.End-currentSlice.Start)/(double)source.SampleRate:0.000}s", 14));
        ButtonRow(("Listen to chop", () => _ = samples.PreviewChopAsync(selectedSampleSlice), !samples.Busy),
            ("Map & prepare kit", () => { samplePage = 1; ShowWorkspace(); }, !samples.Busy));
        Section("sample.navigation","Zoom & navigate",()=> {
        ButtonRow(("Zoom +", () => Navigate(() => viewport.Zoom(2, samples.Cursor)), !samples.Busy), ("Zoom -", () => Navigate(() => viewport.Zoom(.5, samples.Cursor)), !samples.Busy));
        ButtonRow(("Earlier", () => Navigate(() => viewport.Pan(-Math.Max(1, viewport.Length/2))), !samples.Busy), ("Later", () => Navigate(() => viewport.Pan(Math.Max(1, viewport.Length/2))), !samples.Busy));
        ButtonRow(("Full WAV", () => Navigate(() => viewport.Show(0, source.Frames)), !samples.Busy), ("Fit trim", () => Navigate(() => viewport.Show(doc.Start, doc.End)), !samples.Busy));
        });
        var cursorLabel = Label($"Cursor {samples.Cursor} frames - {samples.Cursor/(double)source.SampleRate:0.000}s; view {viewport.Start}-{viewport.End}", 13);
        var position = new SeekBar(this) { Max = 10000, Progress = (int)Math.Clamp((samples.Cursor-viewport.Start)*10000/viewport.Length, 0, 10000), ContentDescription = "Cursor within visible waveform", Enabled = !samples.Busy };
        void Cursor(long frame) { samples.Cursor = frame; waveform.Cursor = frame; waveform.Invalidate(); cursorLabel.Text = $"Cursor {frame} frames - {frame/(double)source.SampleRate:0.000}s"; }
        waveform.PositionChosen += frame => { Cursor(frame); position.Progress = (int)((frame-viewport.Start)*10000/viewport.Length); };
        position.ProgressChanged += (_, a) => { if (a.FromUser) Cursor(viewport.FrameAt(a.Progress/10000.0)); };
        content.AddView(cursorLabel); content.AddView(position);
        content.AddView(sampleStatusLabel);
        if (samples.Busy) { var cancel = new global::Android.Widget.Button(this) { Text = "Cancel transient analysis" }; cancel.Click += (_, _) => samples.CancelChopAnalysis(); content.AddView(cancel); }
        Section("sample.transients", "Transient chop proposals", () => {
            var sensitivity = new SeekBar(this) { Max = 100, Progress = (int)(transientSensitivity*100), ContentDescription = "Transient sensitivity" };
            sensitivity.ProgressChanged += (_, a) => transientSensitivity = a.Progress/100.0;
            content.AddView(Label("Sensitivity - low to high", 13)); content.AddView(sensitivity);
            int[] spacingValues = [20, 40, 80, 160, 320];
            var spacing = new Spinner(this); spacing.Adapter = new ArrayAdapter<string>(this, global::Android.Resource.Layout.SimpleSpinnerDropDownItem, spacingValues.Select(v => $"Minimum {v} ms between chops").ToArray());
            spacing.SetSelection(Array.IndexOf(spacingValues, transientSpacing)); spacing.ItemSelected += (_, a) => transientSpacing = spacingValues[a.Position]; content.AddView(spacing);
            Button("Preview transient boundaries", () => _ = samples.ProposeChopsAsync(transientSensitivity, transientSpacing));
            if (samples.ChopProposal is {} proposal) {
                content.AddView(Label($"{proposal.Boundaries.Count} orange boundaries. Apply replaces existing chops.", 13));
                ButtonRow(("Apply - replace chops", samples.ApplyChopProposal, !samples.Busy), ("Discard", samples.DiscardChopProposal, !samples.Busy));
            }
        });
        Section("sample.trim", "Trim & equal chopping", () => {
            ButtonRow(("Start here", () => samples.Edit(d => d.Trim(samples.Cursor, d.End)), !samples.Busy), ("End here", () => samples.Edit(d => d.Trim(d.Start, samples.Cursor)), !samples.Busy));
            Button("Reset trim", () => samples.Edit(d => d.Trim(0, d.Source.Frames)));
            var parts = new Spinner(this); parts.Adapter = new ArrayAdapter<string>(this, global::Android.Resource.Layout.SimpleSpinnerDropDownItem, Enumerable.Range(1,16).Select(i => $"{i} equal slices").ToArray());
            parts.SetSelection(equalSliceCount-1); parts.ItemSelected += (_, a) => equalSliceCount = a.Position+1; content.AddView(parts);
            Button("Replace chops with equal slices", () => samples.Edit(d => d.EqualParts(equalSliceCount)));
            Button("Export trimmed WAV", () => ExportWave(source, doc.Start, doc.End, "sloop-trim.wav"));
        });
        var slices = doc.Slices(); content.AddView(Label("Chops - select and audition", 18));
        for (int row = 0; row < (slices.Length+3)/4; row++) {
            var pads = new LinearLayout(this) { Orientation = Orientation.Horizontal };
            for (int col = 0; col < 4; col++) {
                int index = row*4+col;
                if (index >= slices.Length) { pads.AddView(new View(this), new LinearLayout.LayoutParams(0,Dp(64),1)); continue; }
                var slice = slices[index];
                var pad = new global::Android.Widget.Button(this) { Text = $"{index+1:00}\n{(slice.End-slice.Start)/(double)source.SampleRate:0.00}s", Activated = index == selectedSampleSlice, Enabled = !samples.Busy, ContentDescription = $"Select and audition chop {index+1}" };
                pad.SetSingleLine(false); pad.SetMaxLines(2); pad.Click += (_, _) => { selectedSampleSlice = index; _ = samples.PreviewChopAsync(index); };
                pads.AddView(pad, new LinearLayout.LayoutParams(0,Dp(64),1));
            } content.AddView(pads);
        }
        var selected = slices[selectedSampleSlice];
        Section("sample.precise", $"Edit selected chop {selectedSampleSlice+1:00} - frames", () => {
            var startField = new EditText(this) { Text = selected.Start.ToString(), InputType = global::Android.Text.InputTypes.ClassNumber, ContentDescription = "Chop start frame inclusive" };
            var endField = new EditText(this) { Text = selected.End.ToString(), InputType = global::Android.Text.InputTypes.ClassNumber, ContentDescription = "Chop end frame exclusive" };
            content.AddView(Label("Start inclusive / end exclusive; adjacent chops share edges",12)); content.AddView(startField); content.AddView(endField);
            Button("Apply exact range", () => samples.Edit(d => { if (!long.TryParse(startField.Text,out var start) || !long.TryParse(endField.Text,out var end)) throw new ArgumentException("Enter whole source frame numbers."); d.SetSliceRange(selectedSampleSlice,start,end); }));
            ButtonRow(("Cursor -1 frame", () => Cursor(Math.Max(0,samples.Cursor-1)), !samples.Busy), ("Cursor +1 frame", () => Cursor(Math.Min(source.Frames-1,samples.Cursor+1)), !samples.Busy));
            ButtonRow(("Start at cursor", () => samples.Edit(d => d.SetSliceRange(selectedSampleSlice,samples.Cursor,selected.End)), !samples.Busy), ("End at cursor", () => samples.Edit(d => d.SetSliceRange(selectedSampleSlice,selected.Start,samples.Cursor)), !samples.Busy));
            ButtonRow(("Move end here", () => samples.Edit(d => { if (!d.MoveBoundary(selected.End,samples.Cursor)) throw new ArgumentException("Choose a frame between neighboring boundaries."); }), selectedSampleSlice < slices.Length-1 && !samples.Busy), ("Remove end - merge next", () => samples.Edit(d => d.RemoveBoundary(selected.End)), selectedSampleSlice < slices.Length-1 && !samples.Busy));
            Button("Fit selected chop", () => Navigate(() => viewport.Show(selected.Start,selected.End)));
        });
        Button($"Export selected chop {selectedSampleSlice+1:00}", () => ExportWave(source,selected.Start,selected.End,$"sloop-slice-{selectedSampleSlice+1:00}.wav"));
        Button("Next - map & send to FM1", () => { samplePage = 1; ShowWorkspace(); });
    }

    // Parent transfer wiring can consume this proposal without changing backup/readback semantics.
    private int? proposedSampleSlot;
    private string? proposedSampleSource;
    private void AddSampleKitReview()
    {
        if (samples.Document is not {} doc) return;
        if (proposedSampleSource != doc.Source.Path) { proposedSampleSource=doc.Source.Path; proposedSampleSlot=null; }
        selectedSampleSlice = Math.Clamp(selectedSampleSlice, 0, doc.Slices().Length-1);
        var options = samples.KitOptions;
        if (kitDraftSource != doc.Source.Path) { kitDraftSource=doc.Source.Path;kitDraftName=options.Name;kitDraftRoot=options.RootNote.ToString(); }
        var ready = samples.Converted is not null;
        var draftMatches = kitDraftName==options.Name && int.TryParse(kitDraftRoot,out var draftRoot) && draftRoot==options.RootNote;
        content.AddView(Label(ready && draftMatches ? "Prepared · review assignments, then choose replacement slot" : "Prepare required · edits, name, root and mapping must match the encoded kit", 14));
        ButtonRow(("Processed chop", () => {
            _ = samples.PreviewChopAsync(selectedSampleSlice);
        }, !samples.Busy), ("Encoded chop", () => _ = samples.PreviewConvertedChopAsync(selectedSampleSlice), ready && !samples.Busy));
        Section("sample.assignments", $"Assignments · {options.Mapping} · selected {selectedSampleSlice+1:00}", () => {
            content.AddView(Label(options.Mapping switch {
                SampleMapping.DrumLanes => "Chop order follows the 16 FM1 drum lane notes. First root is ignored.",
                SampleMapping.Instrument => "Consecutive roots split the keyboard into ranges. Pitch changes playback speed and duration.",
                _ => "Each chop has one consecutive MIDI trigger key, starting at the first root."
            }, 13));
            try {
                foreach (var assignment in SampleKitPlan.Assignments(options,doc.Slices().Length,doc,samples.ChopOptions)) {
                    int chop = assignment.ChopIndex;
                    var pad = new global::Android.Widget.Button(this) {
                        Text = assignment.Description, Enabled = !samples.Busy,
                        ContentDescription = assignment.Description + ". Select and audition processed chop."
                    };
                    pad.Click += (_,_) => { selectedSampleSlice = chop; _ = samples.PreviewChopAsync(chop); };
                    content.AddView(pad);
                }
            } catch (ArgumentException e) { content.AddView(Label(e.Message,14)); }
        });
        // This is a proposal only; the existing upload chooser remains authoritative until wired.
        var target = new global::Android.Widget.Button(this) {
            Text = proposedSampleSlot is {} slot ? $"Proposed destination · USR{slot+1}" : "Propose destination · USR1–4",
            Enabled = !samples.Busy && !connection.Snapshot.Busy
        };
        target.Click += (_,_) => new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Propose slot to replace")!
            .SetItems(new[] { "USR1", "USR2", "USR3", "USR4" }, (_,a) => { proposedSampleSlot = a.Which; ShowWorkspace(); })!
            .SetNegativeButton("Cancel",(_,_)=>{})!.Show();
        content.AddView(target);
        content.AddView(Label("Sending backs up and replaces a slot. Confirm the destination again in Send; stop the FM1 song manually. App playback must also be stopped.",12));
    }

    private void ConfirmProposedSampleSlot(string title, string message, Action<int> send)
    {
        if (proposedSampleSlot is not {} slot) {
            ConfirmSlot(title,message,(_,a) => { proposedSampleSlot=a.Which; send(a.Which); }); return;
        }
        new global::Android.App.AlertDialog.Builder(this)!.SetTitle($"Replace USR{slot+1}?")!
            .SetMessage(message + $"\nProposed destination: USR{slot+1}. Its current contents will be backed up before replacement.")!
            .SetPositiveButton($"Back up & replace USR{slot+1}",(_,_)=>send(slot))!
            .SetNeutralButton("Other slot",(_,_)=>ConfirmSlot(title,message,(_,a)=> { proposedSampleSlot=a.Which;send(a.Which); }))!
            .SetNegativeButton("Cancel",(_,_)=>{})!.Show();
    }
}

internal sealed class SampleWaveView : View
{
    private readonly SampleDocument document;
    private float[] peaks;
    private readonly SampleViewport viewport;
    private readonly CancellationTokenSource peakLoad = new();
    public long Cursor { get; set; }
    public int SelectedSlice { get; set; } = -1;
    public IReadOnlyList<long> ProposedBoundaries { get; set; } = [];
    private readonly Paint paint = new() { AntiAlias = true, StrokeWidth = 2 };
    public event Action<long>? PositionChosen;
    public SampleWaveView(Context context, SampleDocument document, float[] peaks, long cursor, SampleViewport viewport) : base(context)
    {
        this.document = document; this.peaks = peaks; Cursor = cursor; this.viewport = viewport;
        ContentDescription = "Sample waveform. Tap or drag to position cursor within visible frames."; Clickable = true;
        if (viewport.Start != 0 || viewport.End != document.Source.Frames) { this.peaks = []; _ = LoadPeaksAsync(); }
    }
    private async Task LoadPeaksAsync()
    {
        var token = peakLoad.Token; long start = viewport.Start, end = viewport.End;
        try { var result = await Task.Run(() => SamplingTools.Peaks(document.Source,start,end,token:token)); if (!token.IsCancellationRequested) { peaks = result; Invalidate(); } }
        catch (OperationCanceledException) { } catch (IOException) { }
    }
    protected override void OnDetachedFromWindow() { peakLoad.Cancel(); base.OnDetachedFromWindow(); }
    protected override void OnDraw(Canvas canvas)
    {
        base.OnDraw(canvas); canvas.DrawColor(Color.Rgb(27,32,40));
        float X(long frame) => (frame-viewport.Start)*Width/(float)viewport.Length;
        var slices = document.Slices();
        if (SelectedSlice >= 0 && SelectedSlice < slices.Length) { paint.Color = Color.Argb(35,62,205,176); canvas.DrawRect(X(slices[SelectedSlice].Start),0,X(slices[SelectedSlice].End),Height,paint); }
        paint.Color = Color.Rgb(62,205,176);
        for (int i = 0; i < peaks.Length; i++) { float x = (i+.5f)*Width/peaks.Length, size = peaks[i]*Height*.45f; canvas.DrawLine(x,Height/2f-size,x,Height/2f+size,paint); }
        paint.Color = Color.Argb(170,10,10,10); canvas.DrawRect(0,0,X(document.Start),Height,paint); canvas.DrawRect(X(document.End),0,Width,Height,paint);
        paint.Color = Color.Yellow; foreach (var marker in document.Markers) canvas.DrawLine(X(marker),0,X(marker),Height,paint);
        paint.Color = Color.Rgb(255,150,50); foreach (var marker in ProposedBoundaries) canvas.DrawLine(X(marker),0,X(marker),Height,paint);
        paint.Color = Color.White; canvas.DrawLine(X(Cursor),0,X(Cursor),Height,paint);
    }
    public override bool OnTouchEvent(MotionEvent? e)
    {
        if (e is null || !Enabled) return false;
        if (e.Action == MotionEventActions.Down) Parent?.RequestDisallowInterceptTouchEvent(true);
        if (e.Action is MotionEventActions.Down or MotionEventActions.Move or MotionEventActions.Up) { PositionChosen?.Invoke(viewport.FrameAt(e.GetX()/Math.Max(1,Width))); if (e.Action == MotionEventActions.Up) PerformClick(); }
        if (e.Action is MotionEventActions.Up or MotionEventActions.Cancel) Parent?.RequestDisallowInterceptTouchEvent(false);
        return true;
    }
    public override bool PerformClick() { base.PerformClick(); return true; }
    protected override void Dispose(bool disposing) { if (disposing) { peakLoad.Cancel(); peakLoad.Dispose(); paint.Dispose(); } base.Dispose(disposing); }
}



