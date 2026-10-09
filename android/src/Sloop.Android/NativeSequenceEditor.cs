using Android.Widget;
using Sloop.Sequencing;
using Sloop.Workstation;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    private HardwarePattern? hardwareBaseline;
    private void AddNativeSequenceEditor()
    {
        content.AddView(Label("FM1 hardware patterns",22));
        bool linked=connection.Snapshot.Device?.ProtocolVersion>=9&&!connection.Snapshot.IsSimulated&&!connection.Snapshot.Busy&&!connection.IsPlaying;
        AsyncButton("Read all FM1 patterns · preserves current local copy",async()=> {
            var pattern=await connection.EditDeviceAsync("Reading native steps, locks and timing…",(c,t)=>HardwarePatterns.Read(c,connection.Snapshot.Device!,editing.Native?.Current as HardwarePattern,t));
            editing.ReadNative(pattern);hardwareBaseline=pattern;editing.SetStatus("Native patterns read. Read temporarily visits tracks and restores selection.");
        },linked);
        if(editing.Native?.Current is not HardwarePattern native)return;
        int index=state.SelectedTrack;var track=native.Tracks[index];int step=editing.Step;
        if(step>=track.Steps.Length){content.AddView(Label("Selected app step is beyond the FM1's 64 native steps. Select a step in app bars 1–4 to edit native material.",14));return;}
        content.AddView(Label($"Step {step+1} · active length {track.Length} · {track.Locks.Length}/24 locks\nMicro {track.Steps[step].Micro.Value}/64 · {track.Steps[step].Fill}",14));
        var selection=new HardwareSelection(track.Id,new(new(step),new(step+1)));
        ActionButton("Edit selected native step through prompts",()=>DescribeNativeCommands(native,selection));
        void Edit(HardwareEdit change)=>editing.EditNative(HardwareEditor.Propose(native,selection,change));
        if(!track.IsDrum){ActionButton("Native selected step: transpose +1",()=>Edit(HardwareEdit.Transpose(1)));ActionButton("Native selected step: transpose −1",()=>Edit(HardwareEdit.Transpose(-1)));}
        ActionButton("Native selected step: level HARD",()=>Edit(HardwareEdit.SetLevel(HitLevel.Hard)));
        ActionButton("Native selected step: level GHOST",()=>Edit(HardwareEdit.SetLevel(HitLevel.Ghost)));
        ActionButton("Native selected step: quantize micro timing",()=>Edit(HardwareEdit.QuantizeMicro()));
        ActionButton("Undo native local edit",()=>editing.NativeHistory(false),editing.Native.CanUndo);
        ActionButton("Redo native local edit",()=>editing.NativeHistory(true),editing.Native.CanRedo);
        AsyncButton("Send native edits to FM1 · verify readback",async()=> {
            var expected=hardwareBaseline??throw new IOException("Read hardware after connecting before sending.");
            var proposal=EditProposal.Between(expected,native,new("Android native edits","manual/1"));
            var verified=await connection.EditDeviceAsync("Comparing and applying native pattern edits…",(c,t)=>HardwarePatterns.Apply(c,connection.Snapshot.Device!,proposal,t));
            hardwareBaseline=verified;editing.SetStatus("Native pattern edits verified. Local undo remains local; send again to restore hardware.");
        },linked&&hardwareBaseline is not null);
        content.AddView(Label("Hardware edits stay local until Send. A connection error can leave partial device changes; read again to reconcile. Local snapshots preserve inactive data and all drum lanes.",14));
    }
}
