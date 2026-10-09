using Android.Widget;
using Sloop.Protocol;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    private Action? cancelMusicalStarterPreview;
    private void CancelMusicalStarterPreview() => cancelMusicalStarterPreview?.Invoke();
    void AddMusicalStarterEditor()
    {
        content.AddView(Label("Firmware's 24 musical starters · four bars · CHORD / BASS / ARP NOTES. Audition preserves patterns. Apply replaces the selected FM1 synth pattern; hardware EDIT + OCT− undoes it.",14));
        bool ready=state.SelectedTrack<3 && connection.Snapshot.Device?.ProtocolVersion>=14 &&
            !connection.Snapshot.IsSimulated && !connection.Snapshot.IsGenericMidi && !connection.Snapshot.Busy && !connection.IsPlaying && !connection.IsRecording;
        AsyncButton("Browse FM1 musical starters…",async()=> {
            var identity=connection.Snapshot.Device!; int track=state.SelectedTrack;
            var bank=await connection.EditDeviceAsync("Reading firmware starters…",(c,t)=>c.ListMusicalStartersAsync(identity,t));
            new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Firmware musical starters")!
                .SetItems(bank.Entries.Select(e=>e.Name).ToArray(),(_,a)=>ShowMusicalStarter(bank,bank.Entries[a.Which],identity,track))!
                .SetNegativeButton("Close",(_,_)=>{})!.Show();
        },ready);
        if(!ready) content.AddView(Label("Connect firmware with musical starter support, select a synth track, and stop app playback.",12));
    }
    void ShowMusicalStarter(MusicalStarterBank bank,MusicalStarter entry,DeviceInfo identity,int track)
    {
        var box=new LinearLayout(this){Orientation=Orientation.Vertical};box.SetPadding(Dp(16),Dp(8),Dp(16),Dp(8));
        Spinner Choice(string label,string[] names,int selected=0) {
            box.AddView(Label(label,14));var view=new Spinner(this);
            view.Adapter=new ArrayAdapter<string>(this,global::Android.Resource.Layout.SimpleSpinnerDropDownItem,names);
            view.SetSelection(selected);box.AddView(view);return view;
        }
        var key=Choice("Key",["C","C♯","D","D♯","E","F","F♯","G","G♯","A","A♯","B"]);
        var scale=Choice("Scale · firmware names",bank.Scales.Select(s=>s.Name).ToArray(),Math.Min(1,bank.Scales.Count-1));
        var octave=Choice("Octave",["−3","−2","−1","0","+1","+2","+3"],3);
        var mode=Choice("Mode",["CHORD","BASS","ARP NOTES"]);
        var lead=new CheckBox(this){Text="Voice leading"};box.AddView(lead);
        EditText Number(string label,int initial) {box.AddView(Label(label,14));var v=new EditText(this){Text=initial.ToString(),InputType=global::Android.Text.InputTypes.ClassNumber|global::Android.Text.InputTypes.NumberFlagSigned};box.AddView(v);return v;}
        var rotate=Number("Rotate (−16…16 steps)",0);var offset=Number("Offset (−8…8 steps)",0);
        var sync=Choice("Syncopation",["Off","Alternating beats","Every beat"]);var feel=Number("Feel (−32…31)",0);
        var message=Label("Select the same track on FM1. Release held keys and stop its transport before audition or apply.",14);box.AddView(message);
        MusicalStarterOptions Options()=>new((byte)track,entry.Id,(byte)key.SelectedItemPosition,(byte)scale.SelectedItemPosition,
            octave.SelectedItemPosition-3,(byte)mode.SelectedItemPosition,lead.Checked,int.Parse(rotate.Text!),int.Parse(offset.Text!),
            (byte)sync.SelectedItemPosition,int.Parse(feel.Text!));
        bool alive=true;var lifetime=new CancellationTokenSource();
        void Guard() {if(!alive || lifetime.IsCancellationRequested || !ReferenceEquals(connection.Snapshot.Device,identity)||state.SelectedTrack!=track || state.Workspace!=Sloop.Core.Workspace.Sequence)throw new IOException("Connection or track changed. Browse starters again.");}
        static void Require(MusicalStarterStatus s) {if(s!=MusicalStarterStatus.Ok)throw new IOException(s switch {
            MusicalStarterStatus.Busy=>"Stop FM1 transport and release live keys first.", MusicalStarterStatus.Stale=>"FM1 track or audition changed. Browse again.",
            MusicalStarterStatus.ConfirmationRequired=>"Replacement confirmation required.",_=>"Firmware refused starter options."});}
        ushort lease=(ushort)Random.Shared.Next(1,16384); CancellationTokenSource? preview=null;
        async Task Stop() {
            preview?.Cancel();preview=null;
            if(ReferenceEquals(connection.Snapshot.Device,identity))try {await connection.MaintainMusicalStarterLeaseAsync(identity,lease,false);}catch(Exception){ /* firmware lease also expires */ }
        }
        CancelMusicalStarterPreview();
        Action cancel=()=>{alive=false;lifetime.Cancel();preview?.Cancel();};
        cancelMusicalStarterPreview=cancel;
        async Task Listen() {
            CancellationTokenSource? owner=null;
            try {
                await Stop();Guard();var options=Options();
                owner=preview=CancellationTokenSource.CreateLinkedTokenSource(lifetime.Token);
                Require(await connection.EditDeviceAsync("Auditioning firmware starter…",(c,t)=>c.PreviewMusicalStarterAsync(options,lease,t)));
                Guard();
                message.Text="Auditioning · press Stop before changing options and listening again. Close stops audition.";
                try {while(!owner.IsCancellationRequested) {
                    await Task.Delay(500,owner.Token);Guard();
                    Require(await connection.MaintainMusicalStarterLeaseAsync(identity,lease,true,owner.Token));
                }}catch(OperationCanceledException){}finally{if(preview==owner){preview=null;await Stop();}}
            }catch(Exception e){message.Text=e.Message;await Stop();}
            finally {owner?.Dispose();}
        }
        var listen=new Button(this){Text="Listen on FM1"};listen.Click+=async(_,_)=>{listen.Enabled=false;try{await Listen();}finally{if(alive)listen.Enabled=true;}};box.AddView(listen);
        var stop=new Button(this){Text="Stop audition"};stop.Click+=async(_,_)=>await Stop();box.AddView(stop);
        var apply=new Button(this){Text="Apply to FM1…"};box.AddView(apply);
        var scroll=new ScrollView(this);scroll.AddView(box);
        var dialog=new global::Android.App.AlertDialog.Builder(this)!.SetTitle(entry.Name)!.SetView(scroll)!.SetNegativeButton("Close",(_,_)=>{})!.Create()!;
        dialog.DismissEvent+=async(_,_)=>{cancel();if(cancelMusicalStarterPreview==cancel)cancelMusicalStarterPreview=null;await Stop();};
        apply.Click+=async(_,_)=> {
            try {await Stop();Guard();var options=Options();options.Encode(2);
                new global::Android.App.AlertDialog.Builder(this)!.SetTitle($"Replace FM1 track {track+1}?")!
                    .SetMessage("Replace all 64 steps, timing, conditions and locks. Patch and tempo stay selected. Hardware EDIT + OCT− restores the prior pattern.")!
                    .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Apply starter",async(_,_)=> {
                        try {Guard();Require(await connection.EditDeviceAsync("Applying firmware starter…",(c,t)=>c.ApplyMusicalStarterAsync(options,true,t)));
                            hardwareBaseline=null;editing.SetStatus($"{entry.Name} applied. Read FM1 patterns to inspect; hardware Undo restores the previous pattern.");dialog.Dismiss();
                        }catch(Exception e){message.Text=e.Message+" Read hardware before retrying if outcome is uncertain.";}
                    })!.Show();
            }catch(Exception e){message.Text=e.Message;}
        };
        dialog.Show();
    }
}
