using Android.App;
using Android.Content;
using Android.Widget;
using System.Collections.Immutable;
using Sloop.Sequencing;
using Sloop.Workstation;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    const int ImportMidiRequest = 419;
    AppPattern? midiImportSource;
    string? midiImportRoot;
    void ImportMidiFile()
    {
        midiImportSource = (AppPattern)editing.Sequence.Current;
        midiImportRoot = EffectiveWorkspaceRoot;
        var intent = new Intent(Intent.ActionOpenDocument);
        intent.AddCategory(Intent.CategoryOpenable); intent.SetType("*/*");
        StartActivityForResult(intent, ImportMidiRequest);
    }
    async Task ImportMidiFileResult(Result resultCode, Intent? data)
    {
        var source = midiImportSource; var root = midiImportRoot;
        midiImportSource = null; midiImportRoot = null;
        if (resultCode != Result.Ok || data?.Data is not {} uri) return;
        try {
            if (source is null || root is null) throw new IOException("Choose Import MIDI again.");
            var file = await Task.Run(() => {
                using var input = ContentResolver!.OpenInputStream(uri) ?? throw new IOException("Cannot open MIDI file.");
                return MidiFile.Read(input);
            });
            void RequireSource() {
                if (IsFinishing || IsDestroyed || root != EffectiveWorkspaceRoot || !ReferenceEquals(source, editing.Sequence.Current))
                    throw new IOException("The sequence or workspace changed. Choose Import MIDI again.");
            }
            RequireSource();
            new AlertDialog.Builder(this)!.SetTitle("Choose MIDI part")!
                .SetItems(file.Parts.Select(p => $"{p.Name} · channel {p.Channel+1} · {p.Notes.Length} notes").ToArray(), (_, a) => {
                    var part = file.Parts[a.Which];
                    new AlertDialog.Builder(this)!.SetTitle("Destination track")!
                        .SetItems(Sloop.Core.WorkstationState.TrackNames.ToArray(), (_, b) => {
                            try { RequireSource(); ReviewMidiPart(file,part,source,b.Which,RequireSource); }
                            catch(Exception e) { editing.SetStatus(e.Message); }
                        })!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
                })!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
        } catch(Exception e) { editing.SetStatus("MIDI import failed: " + e.Message); }
    }
    void ReviewMidiPart(MidiFileDocument file, MidiFilePart part, AppPattern source, int destination, Action requireSource)
    {
        int channel = destination == 3 ? 9 : destination;
        var after = MidiFile.ToApp(file,part,source,channel);
        var preview = after with { Notes = after.Notes.Where(n => n.Channel == channel)
            .Select(n=>connection.Snapshot.IsGenericMidi?n with{Channel=genericSequenceChannels[destination]}:n).ToImmutableArray() };
        var panel = new LinearLayout(this) {Orientation=Orientation.Vertical};
        var scroll = new ScrollView(this); scroll.AddView(panel);
        panel.AddView(Label($"{part.Name} · {part.Notes.Length} notes · {file.Tempo} BPM\nDestination: {Sloop.Core.WorkstationState.TrackNames[destination]}\nApp import replaces this track's MIDI channel; other channels remain. Original note timing and lengths are retained. Undo is available.\n\n" + string.Join("\n",file.Warnings),14));
        var play = new Button(this) {Text="Preview MIDI part",Enabled=connection.CanPerform};
        play.Click += async (_,_) => {try { requireSource(); await connection.LoopPatternAsync(preview,file.Tempo); }catch(Exception e) { editing.SetStatus(e.Message); }}; panel.AddView(play);
        var stop = new Button(this) {Text="Stop preview"}; stop.Click+=(_,_)=>connection.StopPlaying(); panel.AddView(stop);
        var hardware = new Button(this) {Text="Review FM1 transfer…",Enabled=connection.Snapshot.Device?.ProtocolVersion>=9&&!connection.Snapshot.IsGenericMidi&&!connection.Snapshot.IsSimulated}; panel.AddView(hardware);
        var dialog = new AlertDialog.Builder(this)!.SetTitle("Review MIDI import")!.SetView(scroll)!
            .SetPositiveButton("Import into app",(_,_)=> {
                try {
                    requireSource(); connection.StopPlaying();
                    if (source.Notes.Any(n => n.Channel==channel && (pianoRoll.Locks.Events.Contains(n.Id)||pianoRoll.Locks.Parts.Contains(n.PartId))))
                        throw new IOException("Unlock this track's protected notes before replacing it.");
                    editing.EditPattern(EditProposal.Between(source,after,new("Import MIDI part","midi-file/1")));
                    editing.Tempo=file.Tempo; connection.SetTransportTempo(file.Tempo);
                    state.SelectTrack(destination); sequenceEditChannels[destination]=channel;
                    editing.SetStatus("MIDI part imported locally. Undo sequence restores previous notes. FM1 unchanged."); ShowWorkspace();
                } catch(Exception e) { editing.SetStatus(e.Message); }
            })!.SetNegativeButton("Cancel",(_,_)=>{})!.Create()!;
        dialog.DismissEvent+=(_,_)=>connection.StopPlaying();
        hardware.Click+=async (_,_)=> {
            hardware.Enabled=false;
            try {
                requireSource(); await connection.StopPlayingAsync();
                requireSource(); if(!dialog.IsShowing)return;
                var deviceSession=connection.MidiDestinationSession;
                void RequireDevice() {
                    requireSource();
                    if(deviceSession is null || !ReferenceEquals(deviceSession,connection.MidiDestinationSession))
                        throw new IOException("The FM1 connection changed. Review the transfer again.");
                }
                var before=await connection.EditDeviceAsync("Reading FM1 before MIDI transfer…",(client,token)=>HardwarePatterns.Read(client,connection.Snapshot.Device!,null,token));
                RequireDevice(); if(!dialog.IsShowing)return;
                var plan=MidiFileHardware.Convert(file,part,before,destination);
                dialog.Dismiss(); ReviewMidiHardware(plan,before,file.Tempo,RequireDevice);
            } catch(Exception e) { editing.SetStatus("MIDI transfer review failed: "+e.Message); hardware.Enabled=true; }
        }; dialog.Show();
    }
    void ReviewMidiHardware(MidiHardwareImport plan, HardwarePattern before, int tempo, Action requireSource)
    {
        var panel=new LinearLayout(this){Orientation=Orientation.Vertical};var scroll=new ScrollView(this);scroll.AddView(panel);
        panel.AddView(Label(plan.Report+"\n\nTransfer uses the FM1's existing tempo. Stop playback first. Save on the FM1 to keep the result through power-off. A failed transfer may be partial; read again to reconcile.",14));
        var play=new Button(this){Text="Preview converted pattern",Enabled=connection.CanPerform};panel.AddView(play);
        play.Click+=async (_,_)=>{try{requireSource();await connection.LoopPatternAsync(plan.Preview,tempo);}catch(Exception e){editing.SetStatus(e.Message);}};
        var stop=new Button(this){Text="Stop preview"};stop.Click+=(_,_)=>connection.StopPlaying();panel.AddView(stop);
        var send=new Button(this){Text="Replace FM1 track · verify"};panel.AddView(send);
        var dialog=new AlertDialog.Builder(this)!.SetTitle("FM1 MIDI conversion")!.SetView(scroll)!.SetNegativeButton("Cancel",(_,_)=>{})!.Create()!;
        dialog.DismissEvent+=(_,_)=>connection.StopPlaying();
        send.Click+=async (_,_)=> {
            send.Enabled=false;play.Enabled=false;
            try {
                requireSource();await connection.StopPlayingAsync();
                requireSource();if(!dialog.IsShowing)return;
                var proposal=EditProposal.Between(before,plan.Pattern,new("Import MIDI to FM1","midi-file/1"));
                await connection.EditDeviceAsync("Applying MIDI pattern and verifying…",(client,token)=>HardwarePatterns.Apply(client,connection.Snapshot.Device!,proposal,token));
                hardwareBaseline=null;
                editing.SetStatus("MIDI transfer verified. Save on FM1 to retain it; Read native patterns to edit it locally.");dialog.Dismiss();
            }catch(Exception e){editing.SetStatus("MIDI transfer failed: "+e.Message);send.Enabled=true;play.Enabled=connection.CanPerform;}
        };dialog.Show();
    }
}
