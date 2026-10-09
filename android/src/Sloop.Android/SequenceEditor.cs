using Android.Widget;
using Android.Views;
using System.Collections.Immutable;
using Sloop.Sequencing;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    private static readonly int[] genericSequenceChannels=[0,1,2,9];
    private static readonly int[] sequenceEditChannels=[0,1,2,9];
    private void AddSequenceEditor()
    {
        var pattern=(AppPattern)editing.Sequence.Current; int track=state.SelectedTrack;
        content.AddView(Label(editing.Status,14));
        if (track == 3) Section("sequence.grooves", "FM1 groove bank", AddDrumGrooveEditor);
        long sixteenth=Math.Max(1,pattern.TicksPerQuarter/4);
        int stepCount=checked((int)((pattern.Length.Value+sixteenth-1)/sixteenth));
        int barCount=(stepCount+15)/16;
        editing.Step=Math.Clamp(editing.Step,0,stepCount-1);
        content.AddView(Label($"App pattern · {barCount} bars",18));
        int channel=sequenceEditChannels[track];
        ActionButton("Describe a sequence · offline recipes",()=>DescribeSequence(pattern,channel));
        Section("sequence.scenes","Scenes & arrangements",AddScenesEditor);
        ActionButton($"Edit notes · channel {channel+1}",()=>new global::Android.App.AlertDialog.Builder(this)!
            .SetTitle("Edit MIDI channel (including recorded overrides)")!
            .SetItems(Enumerable.Range(1,16).Select(i=>$"Channel {i}").ToArray(),(_,a)=>{sequenceEditChannels[track]=a.Which;ShowWorkspace();})!.Show());
        if(connection.Snapshot.IsGenericMidi)
            ActionButton($"Output · channel {genericSequenceChannels[track]+1}",()=>new global::Android.App.AlertDialog.Builder(this)!
                .SetTitle($"{Sloop.Core.WorkstationState.TrackNames[track]} MIDI output")!
                .SetItems(Enumerable.Range(1,16).Select(i=>$"Channel {i}").ToArray(),(_,a)=>{genericSequenceChannels[track]=a.Which;ShowWorkspace();})!.Show());
        var notes=pattern.Notes.Where(n=>n.Channel==channel).ToArray();
        AddPianoRoll(pattern,channel);
        var bars=new LinearLayout(this){Orientation=Orientation.Horizontal};
        int selectedBar=editing.Step/16;
        for(int bar=0;bar<barCount;bar++) {
            if(bar>0&&bar%4==0){content.AddView(bars);bars=new LinearLayout(this){Orientation=Orientation.Horizontal};}
            int chosen=bar;
            var b=new Button(this){Text=$"Bar {bar+1}",Activated=bar==selectedBar};
            b.Click+=(_,_)=>{editing.Step=chosen*16;ShowWorkspace();};
            bars.AddView(b,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WrapContent,1));
        }content.AddView(bars);
        for(int row=0;row<2;row++){
            var line=new LinearLayout(this){Orientation=Orientation.Horizontal};
            for(int col=0;col<8;col++) {int step=selectedBar*16+row*8+col;
                var b=new Button(this){Text=notes.Any(n=>n.Start.Value==step*sixteenth)?"●":(step+1).ToString(),Enabled=step<stepCount,Activated=step==editing.Step,ContentDescription=$"Sequence step {step+1}"};
                b.Click+=(_,_)=>{editing.Step=step;ShowWorkspace();};line.AddView(b,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WrapContent,1));
            } content.AddView(line);
        }
        content.AddView(Label($"Selected step {editing.Step+1} · track {track+1}",18));
        NumberControl("MIDI note",editing.Pitch,0,127,v=>{editing.Pitch=v;ShowWorkspace();});
        Section("sequence.note", "Velocity, duration & tempo", () => {
        NumberControl("Velocity",editing.Velocity,1,127,v=>editing.Velocity=v);
        NumberControl("Duration (sixteenths)",checked((int)Math.Max(1,editing.Duration/sixteenth)),1,16,v=>editing.Duration=checked((int)(v*sixteenth)));
        NumberControl("Tempo (BPM)",editing.Tempo,30,240,v=>{editing.Tempo=v;connection.SetTransportTempo(v);});
        });
        var onset=editing.Step*sixteenth;
        var existing=notes.FirstOrDefault(n=>n.Start.Value==onset&&n.Pitch==editing.Pitch);
        ActionButton(existing is null?"Add note at selected step":"Update note at selected step",()=> {
            var id=existing?.Id??Guid.NewGuid();
            editing.EditPattern(AppEditor.PutNote(pattern,id,new(id,pattern.Id,new(onset),new(Math.Min(editing.Duration,pattern.Length.Value-onset)),editing.Pitch,editing.Velocity,channel),pianoRoll.Locks));
        });
        if(existing is not null)ActionButton("Remove selected note",()=>editing.EditPattern(AppEditor.PutNote(pattern,existing.Id,null,pianoRoll.Locks)));
        Section("sequence.edit", "Transform & undo", () => {
        var selection=new AppSelection(notes.Select(n=>n.Id).ToImmutableHashSet());
        ActionButton("Transpose this track +1",()=>editing.EditPattern(AppEditor.Propose(pattern,selection,AppEdit.Transpose(1),pianoRoll.Locks)));
        ActionButton("Transpose this track −1",()=>editing.EditPattern(AppEditor.Propose(pattern,selection,AppEdit.Transpose(-1),pianoRoll.Locks)));
        ActionButton("Quantize this track to sixteenths",()=>editing.EditPattern(AppEditor.Propose(pattern,selection,AppEdit.Quantize(new(sixteenth)),pianoRoll.Locks)));
        ActionButton("Thin this track · keep every second note",()=>editing.EditPattern(AppEditor.Propose(pattern,selection,AppEdit.Variation(2,42),pianoRoll.Locks)));
        ActionButton("Undo sequence",()=>editing.PatternHistory(false),editing.Sequence.CanUndo);
        ActionButton("Redo sequence",()=>editing.PatternHistory(true),editing.Sequence.CanRedo);
        });
        AsyncButton("Loop app pattern through MIDI",()=>connection.LoopPatternAsync(pattern with{Notes=System.Collections.Immutable.ImmutableArray.CreateRange(
            connection.Snapshot.IsGenericMidi?pattern.Notes.Select(n=>n with{Channel=n.Channel switch{0=>genericSequenceChannels[0],1=>genericSequenceChannels[1],2=>genericSequenceChannels[2],9=>genericSequenceChannels[3],_=>n.Channel}}):pattern.Notes)},
            editing.Tempo),connection.CanPerform&&pattern.Notes.Length>0);
        ActionButton("Stop app pattern",()=>connection.StopPlaying());
        Section("sequence.notes", $"Note list · {notes.Length} notes", () => content.AddView(Label(string.Join("\n",notes.OrderBy(n=>n.Start.Value).Select(n=>$"Step {n.Start.Value/sixteenth+1}: note {n.Pitch}, velocity {n.Velocity}, {n.Duration.Value/(double)sixteenth:0.##} steps")),14)));
        Section("sequence.device", "FM1 hardware pattern", AddNativeSequenceEditor);
    }
    private void DescribeSequence(AppPattern pattern,int channel)=>DescribePatternCommands(pattern,channel);
}
