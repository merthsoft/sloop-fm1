using Android.Graphics;
using Android.Media;
using Android.Views;
using Android.Widget;
using Orientation=Android.Widget.Orientation;
using Sloop.Core.Sampling;
using Sloop.SampleEncoding;
using Sloop.Workstation;
namespace Sloop.Android;

public sealed partial class MainActivity
{
    private bool sampleAdvanced,quickSound=true,quickModeChosen;
    private string? quickSource,quickChopSource;
    private int? quickInput,quickReadyTrack;
    private SlotArtifact? quickReadyKit;
    private string quickName="";
    private void AddSampleEditor()
    {
        if(sampleAdvanced) {
            ButtonRow(("← Simple sampler",()=>{sampleAdvanced=false;ShowWorkspace();},!samples.Busy));
            AddAdvancedSampleEditor();return;
        }
        if(connection.Snapshot.Busy){AddAdvancedSampleEditor();return;}
        if(samples.Document is {} current&&quickSource!=current.Source.Path) {
            if(quickSource is null&&!quickModeChosen)quickSound=samples.KitOptions.Mapping==SampleMapping.Instrument||current.Markers.Count==0;
            quickSource=current.Source.Path;quickName=samples.KitOptions.Name=="SLOOP"?"":samples.KitOptions.Name;kitTransferStatus="";
        }
        void ActionButton(string title,Action action,bool enabled=true) {
            var b=new Button(this){Text=title,Enabled=enabled&&!samples.Busy};b.Click+=(_,_)=>action();content.AddView(b);
        }
        void Mode(bool sound){quickModeChosen=true;quickSound=sound;ShowWorkspace();}
        var modes=new LinearLayout(this){Orientation=Orientation.Horizontal};
        foreach(bool sound in new[]{true,false}){var b=new Button(this){Text=sound?"Playable sound":"Chopped loop",Activated=sound==quickSound,Enabled=!samples.Busy};b.Click+=(_,_)=>Mode(sound);modes.AddView(b,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WrapContent,1));}content.AddView(modes);
        content.AddView(Label(quickSound?"A voice, a meow, a single note — play it up and down the piano.":"Trim a phrase, then divide it into chops you can play.",13));
        captureUiActive=samples.RecordingActive;captureMeter=null;
        sampleStatusLabel=Label(samples.Status.Split('\n')[0],13);
        if(captureUiActive) {
            content.AddView(sampleStatusLabel);
            captureMeter=new ProgressBar(this,null,global::Android.Resource.Attribute.ProgressBarStyleHorizontal){Max=32768,Progress=samples.CapturePeak};content.AddView(captureMeter);
            var stop=new Button(this){Text="Stop · keep recording"};stop.Click+=(_,_)=>samples.StopRecording();content.AddView(stop);return;
        }
        ButtonRow(("● Record",BeginQuickRecording,!samples.Busy),("Import WAV",()=>{quickModeChosen=true;ImportWave();},!samples.Busy));
        if(samples.Document is not {} doc) {
            content.AddView(sampleStatusLabel);content.AddView(Label("Record first. Your original is kept, so you can always change the trim or chops.",14));
            ActionButton("Sample library",BrowseRetainedSamples);return;
        }
        if(!quickSound&&quickChopSource!=doc.Source.Path&&doc.Markers.Count==0&&doc.End-doc.Start>=4) {
            quickChopSource=doc.Source.Path;samples.Edit(d=>d.EqualParts(4));
        }
        if(!ReferenceEquals(viewportDocument,doc)){viewportDocument=doc;sampleViewport=new(doc.Source.Frames);}
        var viewport=sampleViewport!;
        selectedSampleSlice=Math.Clamp(selectedSampleSlice,0,doc.Slices().Length-1);
        content.AddView(Label($"{(doc.End-doc.Start)/(double)doc.Source.SampleRate:0.00} seconds kept · drag Start and End",14));
        var wave=new SampleWaveView(this,doc,samples.Peaks,samples.Cursor,viewport){TrimHandles=true,ShowChops=!quickSound,ScaleQuietWaveform=true,SelectedSlice=quickSound?-1:selectedSampleSlice,Enabled=!samples.Busy,ContentDescription="Waveform. Drag the green Start or amber End handle to trim. Tap elsewhere to select a split position."};
        wave.TrimChosen+=(start,end)=>samples.Edit(d=> {
            var equal=QuickSampling.EqualCount(d);
            if(!quickSound&&equal is {} count&&end-start>=count)d.TrimAndEqualParts(start,end,count);else d.Trim(start,end);
        });
        wave.PositionChosen+=frame=>{samples.Cursor=frame;wave.Cursor=frame;wave.Invalidate();};
        content.AddView(wave,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MatchParent,Dp(180)));
        ButtonRow(("Listen",()=>_=samples.PreviewAsync(doc.Start,doc.End),!samples.Busy),("Stop",samples.StopPreview,true),("Undo",()=>samples.Edit(d=>d.Undo()),doc.CanUndo&&!samples.Busy));
        if(quickSound) {
            content.AddView(Label("Try its pitch · C is the original recording",13));
            int[] offsets=[0,2,4,5,7,9,11,12];string[] keys=["C","D","E","F","G","A","B","C ↑"];
            for(int row=0;row<2;row++) {
                var pads=new LinearLayout(this){Orientation=Orientation.Horizontal};
                for(int col=0;col<4;col++){int i=row*4+col;var b=new Button(this){Text=keys[i],Enabled=!samples.Busy};b.Click+=(_,_)=>_=samples.PreviewAsync(doc.Start,doc.End,playbackRate:Math.Pow(2,offsets[i]/12.0));pads.AddView(b,new LinearLayout.LayoutParams(0,Dp(54),1));}
                content.AddView(pads);
            }
        } else {
            var count=QuickSampling.EqualCount(doc);
            content.AddView(Label($"{doc.Slices().Length} chops · tap a pad to listen",14));
            ButtonRow(("4 chops",()=>samples.Edit(d=>d.TrimAndEqualParts(d.Start,d.End,4)),!samples.Busy),("8 chops",()=>samples.Edit(d=>d.TrimAndEqualParts(d.Start,d.End,8)),!samples.Busy),("16 chops",()=>samples.Edit(d=>d.TrimAndEqualParts(d.Start,d.End,16)),!samples.Busy));
            for(int row=0;row<(doc.Slices().Length+3)/4;row++) {
                var pads=new LinearLayout(this){Orientation=Orientation.Horizontal};
                for(int col=0;col<4;col++){int i=row*4+col;if(i>=doc.Slices().Length)break;var slice=doc.Slices()[i];var b=new Button(this){Text=$"{i+1}\n{(slice.End-slice.Start)/(double)doc.Source.SampleRate:0.##}s",Enabled=!samples.Busy};StyleButton(b,i==selectedSampleSlice);b.Click+=(_,_)=>{selectedSampleSlice=i;_=samples.PreviewAsync(slice.Start,slice.End);};pads.AddView(b,new LinearLayout.LayoutParams(0,Dp(60),1));}content.AddView(pads);
            }
            content.AddView(Label("Four chops divide what you trimmed — four beats or four measures, your choice.",12));
            ButtonRow(("Split at cursor",()=>samples.Edit(d=>{if(!d.Split(samples.Cursor))throw new ArgumentException("Tap inside the waveform first. Up to 16 chops.");}),!samples.Busy));
        }
        var name=new EditText(this){Text=quickName,Hint=quickSound?"Sound name (optional)":"Loop name (optional)"};name.SetSingleLine(true);name.TextChanged+=(_,_)=>quickName=name.Text??"";content.AddView(name);
        var view=QuickSampling.View(doc,quickSound);
        try {
            int rate=QuickSampling.FittingRate(view,samples.ChopOptions);
            if(rate<22050)content.AddView(Label("Uses lower-detail audio to fit the whole phrase. Pitch and timing stay the same.",13));
            ActionButton("Send to FM1",()=>_=PrepareAndSendQuickSample());
        } catch(ArgumentException e){content.AddView(Label(e.Message,14));}
        if(samples.KitOptions.Mapping==(quickSound?SampleMapping.Instrument:SampleMapping.Chops)&&samples.QuickConverted is {} ready&&ReferenceEquals(ready.Artifact,quickReadyKit)&&quickReadyTrack is {} track)
            ActionButton("Play on keyboard",()=>{StopPerformance();state.SelectTrack(track);performOptions=performOptions with{Surface=PerformSurface.Keyboard,Key=0,Octave=quickSound?4:3,Style=PlayStyle.Block,Latch=false,BassTrack=-1,FollowHardwareOctave=false};state.Workspace=Sloop.Core.Workspace.Perform;ShowWorkspace();});
        if(!string.IsNullOrEmpty(kitTransferStatus))content.AddView(Label(kitTransferStatus,13));
        content.AddView(sampleStatusLabel);
        if(samples.PreparingKit){var cancel=new Button(this){Text="Cancel preparation"};cancel.Click+=(_,_)=>samples.CancelConversion();content.AddView(cancel);}
        Section("sample.simple.more","More options",()=> {
            ActionButton("Recording input",ChooseQuickInput);
            ActionButton($"Listen through · {samples.OutputName}",ChoosePreviewOutput);
            ButtonRow(("Fit trim",()=>{viewport.Show(doc.Start,doc.End);ShowWorkspace();},!samples.Busy),("Whole recording",()=>{viewport.Show(0,doc.Source.Frames);ShowWorkspace();},!samples.Busy));
            ActionButton("Sample library",BrowseRetainedSamples);
            ActionButton("Advanced editor",()=>{sampleAdvanced=true;ShowWorkspace();});
        });
    }
    private void BeginQuickRecording()
    {
        quickModeChosen=true;
        var inputs=samples.Inputs();var input=inputs.FirstOrDefault(d=>d.Id==quickInput)??inputs.FirstOrDefault(d=>d.Type==AudioDeviceType.BuiltinMic)??inputs.FirstOrDefault();
        if(input is null){Toast.MakeText(this,"No microphone or USB input found.",ToastLength.Long)!.Show();return;}
        samples.PendingCaptureInput=input.Id;
        if(CheckSelfPermission(global::Android.Manifest.Permission.RecordAudio)!=global::Android.Content.PM.Permission.Granted)RequestPermissions([global::Android.Manifest.Permission.RecordAudio],RecordingPermissionRequest);else StartPendingRecording();
    }
    private void ChooseQuickInput()
    {
        var inputs=samples.Inputs();
        new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Recording input")!.SetItems(inputs.Select(d=>d.Type==AudioDeviceType.BuiltinMic?"Phone microphone":$"USB · {d.ProductName}").ToArray(),(_,a)=>{quickInput=inputs[a.Which].Id;ShowWorkspace();})!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
    }
    private async Task PrepareAndSendQuickSample()
    {
        if(samples.Document is not {} doc)return;
        long request=sampleUiRequest.Begin(),revision=doc.Revision;string root=EffectiveWorkspaceRoot;
        bool mode=quickSound;
        bool Current()=>CurrentSampleRequest(request,doc,root)&&doc.Revision==revision&&quickSound==mode&&state.Workspace==Sloop.Core.Workspace.Sample;
        try {
        var result=await samples.PrepareQuickAsync(quickSound,quickName);if(result is null)return;
        if(!Current()||!ReferenceEquals(samples.QuickConverted?.Artifact,result.Artifact))return;
        if(connection.Snapshot.Device is null||connection.Snapshot.IsSimulated||connection.Snapshot.IsGenericMidi){
            new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Ready for your FM1")!.SetMessage("Your samples are prepared and the original is kept. Connect the FM1, then tap Send to FM1 again.")!.SetPositiveButton("Connect",(_,_)=>ShowConnectionMenu())!.SetNegativeButton("Later",(_,_)=>{})!.Show();return;
        }
        var panel=new LinearLayout(this){Orientation=Orientation.Vertical};
        panel.AddView(Label("Replace user slot · its old samples will be backed up",13));
        var slots=new Spinner(this);slots.Adapter=new ArrayAdapter<string>(this,global::Android.Resource.Layout.SimpleSpinnerDropDownItem,new[]{"Slot 1 · USR1","Slot 2 · USR2","Slot 3 · USR3","Slot 4 · USR4"});slots.SetSelection(proposedSampleSlot??0);panel.AddView(slots);
        panel.AddView(Label("Play on synth · replaces its instrument, keeps its sequence",13));
        var synth=new Spinner(this);synth.Adapter=new ArrayAdapter<string>(this,global::Android.Resource.Layout.SimpleSpinnerDropDownItem,new[]{"Synth 1","Synth 2","Synth 3"});synth.SetSelection(Math.Min(state.SelectedTrack,2));panel.AddView(synth);
        var destination=connection.MidiDestinationSession;
        new global::Android.App.AlertDialog.Builder(this)!.SetTitle(quickSound?"Send playable sound":"Send chopped loop")!.SetView(panel)!
            .SetMessage(quickSound?"Middle C plays the original. The other piano keys change its pitch.":$"{result.Artifact.Preview.Count} chops play on consecutive keys starting at C3.")!
            .SetPositiveButton("Send & play",async(_,_)=> {
                try {
                    if(!Current()||!ReferenceEquals(destination,connection.MidiDestinationSession))throw new IOException("Sample or destination changed. Choose Send again.");
                    StopPerformance();proposedSampleSlot=slots.SelectedItemPosition;await SendSampleKitAsync(result.Artifact,slots.SelectedItemPosition,true,synth.SelectedItemPosition);
                }catch(Exception error){editing.SetStatus("Sample send failed: "+error.Message);}
            })!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
        }catch(Exception error){editing.SetStatus("Sample review failed: "+error.Message);}
    }
}
