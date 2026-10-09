using Android.App;
using Android.Widget;
using Sloop.Android.Services;
using Sloop.Workstation;

namespace Sloop.Android;

public sealed partial class MainActivity
{
    AndroidMidiInput? midiInput;
    MidiInputDecoder? midiDecoder;
    ExternalMidiNotes? midiNotes;
    bool controllerChords;
    string controllerStatus="External controller off";
    Button? controllerButton;
    object? controllerDestinationSession;
    void SynchronizeMidiInputDestination()
    {
        var session=connection.MidiDestinationSession;
        if(ReferenceEquals(controllerDestinationSession,session))return;
        controllerDestinationSession=session;CloseMidiInput();
    }
    void AddMidiInputControl()
    {
        controllerButton=new Button(this){Text=controllerStatus,TextSize=12};
        controllerButton.Click+=(_,_)=>ShowMidiInputMenu();content.AddView(controllerButton);
    }
    void SetControllerStatus(string value){controllerStatus=value;if(controllerButton is not null)controllerButton.Text=value;}
    void ResetMidiInputNotes(){midiInput?.DiscardPending();midiNotes?.Reset();midiDecoder?.Reset();}
    void CloseMidiInput(){midiInput?.Close();ResetMidiInputNotes();SetControllerStatus("External controller off");}
    void DisposeMidiInput(){CloseMidiInput();midiInput?.Dispose();midiInput=null;}
    void ShowMidiInputMenu()
    {
        new AlertDialog.Builder(this)!.SetTitle("External controller input")!.SetItems(new[]{"Choose device / output port",controllerChords?"Mode: chord roots (change to literal)":"Mode: literal keyboard (change to chord roots)","Disconnect controller"},(_,a)=> {
            if(a.Which==0)ChooseMidiInput();
            else if(a.Which==1){ResetMidiInputNotes();controllerChords=!controllerChords;SetControllerStatus("Controller mode: "+(controllerChords?"chord roots":"literal pitches"));}
            else CloseMidiInput();
        })!.SetNegativeButton("Close",(_,_)=>{})!.Show();
    }
    void ChooseMidiInput()
    {
        try {
            midiNotes??=new ExternalMidiNotes(HoldMidiInput,owner=>connection.Performer.Release(owner));
            midiDecoder??=new MidiInputDecoder(m=>midiNotes.Receive(m));
            midiInput??=new AndroidMidiInput(this,bytes=> {
                if(state.Workspace!=Sloop.Core.Workspace.Perform||!connection.CanPerform){ResetMidiInputNotes();return;}
                PerformSafely(()=>midiDecoder.Feed(bytes));
            },message=>{ResetMidiInputNotes();SetControllerStatus(message);});
            var candidates=midiInput.Candidates(connection.MidiDestinationDeviceId);
            if(candidates.Length==0){SetControllerStatus("No external MIDI output port found (destination excluded).");return;}
            new AlertDialog.Builder(this)!.SetTitle("Controller device / output port")!.SetItems(candidates.Select(c=>c.Name).ToArray(),(_,a)=> {
                ResetMidiInputNotes();SetControllerStatus("Opening controller…");
                midiInput.Open(candidates[a.Which],name=>SetControllerStatus("Controller: "+name));
            })!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
        }catch(Exception e){SetControllerStatus(e.Message);}
    }
    void HoldMidiInput(string owner,int pitch,int velocity)
    {
        var o=performOptions;
        if(!controllerChords){connection.Performer.Hold(owner,[new(PerformChannel,pitch)],velocity,PlayStyle.Block);return;}
        if(state.SelectedTrack==3)return;
        var chord=ExternalMidiHarmony.Chord(pitch,o.Key,o.Scale,performShape,o.Inversion);
        if(chord is null)return;
        LiveNote? bass=o.BassTrack>=0&&o.BassTrack!=state.SelectedTrack?new(o.BassTrack,chord.Bass):null;
        connection.Performer.Hold(owner,chord.Notes.Select(n=>new LiveNote(PerformChannel,n)).ToArray(),velocity,o.Style,o.Tempo,o.Division,o.StrumMs,bass);
    }
}
