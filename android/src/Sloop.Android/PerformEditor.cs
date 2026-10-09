using Android.Graphics;
using Android.Views;
using Android.Widget;
using Sloop.Workstation;
using Sloop.SampleEncoding;
using Sloop.Sequencing;
namespace Sloop.Android;

public sealed partial class MainActivity
{
    static PerformOptions performOptions=new();
    PerformanceMapping Mapping=>performOptions.Mapping??new();
    PerformanceMacro Macro=>performOptions.Macro??new();
    PerformanceXyView? xySurface;
    readonly Dictionary<int,(int Cell,int Velocity)> performFingers=[];
    ChordShape performShape;
    int? latchedDegree;
    int? latchedOctave;
    ChordJoystickView? chordJoystick;
    int[]? previousVoicing;
    TextView? performReadout;
    TextView? performanceRegister;
    PerformanceSurfaceView? playingSurface;
    string performanceCheck="Use plain-note settings on FM1; Check destination detects extra hardware transforms.";
    int PerformChannel=>connection.Snapshot.IsGenericMidi&&performOptions.ChannelOverride>=0?performOptions.ChannelOverride:state.SelectedTrack==3?9:state.SelectedTrack;
    int PerformOctave=>PerformanceHarmony.EffectiveOctave(performOptions.Octave,connection.HardwareOctave,performOptions.FollowHardwareOctave);
    string RegisterDescription=> $"{PerformanceHarmony.Names[performOptions.Key]} {performOptions.Scale} · oct {PerformOctave}\nBase {performOptions.Octave} · "+
        (connection.Snapshot.IsGenericMidi?"generic MIDI":performOptions.FollowHardwareOctave ? connection.HardwareOctave is {} offset ? $"FM1 {offset:+0;-0;0}" : "FM1 unavailable" : "app only");
    void OnHardwareOctaveChanged()
    {
        if(state.Workspace!=Sloop.Core.Workspace.Perform||!performOptions.FollowHardwareOctave)return;
        if(performanceRegister is not null)performanceRegister.Text=RegisterDescription;
        playingSurface?.UpdateLabels(PerformanceLabels());
        if(performOptions.Surface==PerformSurface.DrumPads)return;
        RevoicePerformanceRegister();
    }
    void ChangePerformOctave(int delta)
    {
        performOptions=performOptions with{Octave=Math.Clamp(performOptions.Octave+delta,1,6)};
        if(performanceRegister is not null)performanceRegister.Text=RegisterDescription;
        playingSurface?.UpdateLabels(PerformanceLabels());
        if(performOptions.Surface!=PerformSurface.DrumPads)RevoicePerformanceRegister();
    }
    void RevoicePerformanceRegister()
    {
        ResetMidiInputNotes();
        previousVoicing=null;
        // A latched chord keeps its sounding register until the next chord press.
        if(performOptions.Surface==PerformSurface.Chords&&performOptions.Latch)return;
        connection.ReleasePerformance();
        PerformSafely(()=> {
            foreach(var pair in performFingers.ToArray())PlayPerformance(pair.Key,pair.Value.Cell,pair.Value.Velocity,true,false);
            if(latchedDegree is {} degree&&performFingers.Count==0)PlayPerformance(-1,degree,100,true);
        });
    }
    string[] PerformanceLabels()
    {
        var o=performOptions;
        return o.Surface switch {
            PerformSurface.Chords=>Enumerable.Range(0,8).Select(d=>$"{new[]{"I","ii","iii","IV","V","vi","vii","I ↑"}[Mapping.Degree(d)]}\n{PerformanceHarmony.Chord(o.Key,o.Scale,PerformOctave,Mapping.Degree(d),ChordShape.Diatonic,0,false).Name}").ToArray(),
            PerformSurface.DrumPads=>new[]{"Kick","Low kick","Snare","Clap","Closed hat","Open hat","Pedal hat","Rim","Elec snare","Low tom","High tom","Crash","Ride","Maracas","Conga","Cowbell"},
            PerformSurface.Keyboard=>Enumerable.Range(0,24).Select(i=>PerformanceHarmony.Names[(12*(PerformOctave+1)+i+o.Key)%12]).ToArray(),
            _=>Enumerable.Range(0,24).Select(i=>{int n=PerformanceHarmony.ScaleNote(o.Key,o.Scale,PerformOctave,i);return $"{PerformanceHarmony.Names[n%12]}{n/12-1}";}).ToArray()};
    }
    void StopPerformance()
    {
        ResetMidiInputNotes();
        ClearHardwarePerformanceTouches();
        performFingers.Clear();latchedDegree=null;latchedOctave=null;previousVoicing=null;performShape=ChordShape.Diatonic;
        chordJoystick?.ClearTouches();xySurface?.ClearTouches();
        try{connection.Macros.Stop();}catch(Exception error){performanceCheck="Macro reset failed: "+error.Message;}
        connection.ReleasePerformance();playingSurface?.ClearTouches();
        if(performReadout is not null)performReadout.Text="Released all app-owned performance notes.";
        FinishPerformRecording();
    }
    void OnPerformanceFailure(Exception error)=>RunOnUiThread(()=> {StopPerformance();if(performReadout is not null)performReadout.Text="Performance stopped: "+error.Message;});
    void PerformSafely(Action action){try{action();}catch(Exception error){OnPerformanceFailure(error);}}
    void AddPerformEditor()
    {
        connection.Performer.Failed-=OnPerformanceFailure;
        connection.EnablePerformanceCapture();
        connection.Performer.Failed-=OnPerformanceFailure;
        connection.Performer.Failed+=OnPerformanceFailure;
        AddMidiInputControl();
        var o=performOptions;
        performReadout=Label(connection.CanPerform?$"{(connection.Snapshot.IsGenericMidi?"Generic MIDI":"FM1 MIDI")} · channel {PerformChannel+1}":"Offline touch preview · connect MIDI for sound",12);performReadout.SetLines(2);content.AddView(performReadout);
        var controls=new LinearLayout(this){Orientation=Orientation.Horizontal};
        void Control(string text,Action action){var b=new Button(this){Text=text,TextSize=12};b.Click+=(_,_)=>action();controls.AddView(b,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WrapContent,1));}
        Control(o.Surface.ToString(),()=>ChoosePerformance("Playing surface",Enum.GetNames<PerformSurface>(),(int)performOptions.Surface,i=>performOptions=performOptions with{Surface=(PerformSurface)i}));
        Control(o.Style.ToString(),()=>ChoosePerformance("Play style",Enum.GetNames<PlayStyle>(),(int)performOptions.Style,i=>performOptions=performOptions with{Style=(PlayStyle)i}));
        if(connection.Snapshot.IsGenericMidi)Control($"Ch {PerformChannel+1}",()=>ChoosePerformance("MIDI output channel",Enumerable.Range(1,16).Select(i=>$"Channel {i}").ToArray(),PerformChannel,i=>performOptions=performOptions with{ChannelOverride=i}));
        Control("Settings",PerformanceSettings);content.AddView(controls);
        ButtonRow(($"Key · {PerformanceHarmony.Names[o.Key]}",()=>ChoosePerformance("Key",PerformanceHarmony.Names,o.Key,
            i=>performOptions=performOptions with{Key=i}),true),
            ($"Scale · {ScaleDisplay(o.Scale)}",()=>ChoosePerformance("Scale",Enum.GetValues<PerformanceScale>().Select(ScaleDisplay).ToArray(),(int)o.Scale,
            i=>performOptions=performOptions with{Scale=(PerformanceScale)i}),true));
        var octaves=new LinearLayout(this){Orientation=Orientation.Horizontal};
        var down=new Button(this){Text="Oct−",Enabled=o.Octave>1};
        var up=new Button(this){Text="Oct+",Enabled=o.Octave<6};
        down.Click+=(_,_)=>{ChangePerformOctave(-1);down.Enabled=performOptions.Octave>1;up.Enabled=performOptions.Octave<6;};
        up.Click+=(_,_)=>{ChangePerformOctave(1);down.Enabled=performOptions.Octave>1;up.Enabled=performOptions.Octave<6;};
        performanceRegister=Label(RegisterDescription,12);
        octaves.AddView(down,new LinearLayout.LayoutParams(Dp(70),ViewGroup.LayoutParams.WrapContent));
        octaves.AddView(performanceRegister,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WrapContent,1));
        octaves.AddView(up,new LinearLayout.LayoutParams(Dp(70),ViewGroup.LayoutParams.WrapContent));
        content.AddView(octaves);
        var safety=new LinearLayout(this){Orientation=Orientation.Horizontal};
        var latch=new CheckBox(this){Text="Latch",Checked=o.Latch};latch.SetTextColor(Color.White);
        latch.CheckedChange+=(_,a)=>{StopPerformance();performOptions=performOptions with{Latch=a.IsChecked};ShowWorkspace();};safety.AddView(latch,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WrapContent,1));
        var voiceLead=new CheckBox(this){Text="Voice lead",Checked=o.VoiceLead};voiceLead.SetTextColor(Color.White);
        voiceLead.CheckedChange+=(_,a)=>performOptions=performOptions with{VoiceLead=a.IsChecked};safety.AddView(voiceLead,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WrapContent,1));content.AddView(safety);
        var release=new Button(this){Text="Release / stop",TextSize=12};release.Click+=(_,_)=>StopPerformance();content.AddView(release);
        string[] labels=PerformanceLabels();
        int columns=o.Surface switch{PerformSurface.Chords=>4,PerformSurface.DrumPads=>4,PerformSurface.Keyboard=>12,PerformSurface.Ribbon=>24,_=>6};
        playingSurface=new PerformanceSurfaceView(this,labels,columns,o.Surface==PerformSurface.Keyboard,o.Surface==PerformSurface.Ribbon);
        playingSurface.Cancelled+=StopPerformance;
        playingSurface.Play+=(pointer,cell,velocity,held)=>PerformSafely(()=>PlayPerformance(pointer,cell,velocity,held));
        AddPerformanceGestureView(playingSurface,o.Surface==PerformSurface.Ribbon?120:160);
        chordJoystick=null;
        if(o.Surface==PerformSurface.Chords) {
            var joystick=new ChordJoystickView(this,Mapping.Directions);chordJoystick=joystick;joystick.Cancelled+=StopPerformance;joystick.Change+=shape=>PerformSafely(()=> {
                if(shape==performShape)return;
                if(shape==ChordShape.InversionDown)performOptions=performOptions with{Inversion=Math.Max(-3,performOptions.Inversion-1)};
                if(shape==ChordShape.InversionUp)performOptions=performOptions with{Inversion=Math.Min(3,performOptions.Inversion+1)};
                performShape=shape;
                foreach(var pair in performFingers.ToArray())PlayPerformance(pair.Key,pair.Value.Cell,pair.Value.Velocity,true,false);
                if(latchedDegree is {} degree&&performFingers.Count==0)PlayPerformance(-1,degree,100,true);
            });AddPerformanceGestureView(joystick,120);
        }
        AddPerformanceMacro();
        Section("perform.recording", "Record performance", () => {
        var recording=new LinearLayout(this){Orientation=Orientation.Horizontal};
        var record=new Button(this){Text="Record gestures",TextSize=12};
        record.Click+=(_,_)=> {
            StopPerformance();
            var panel=new LinearLayout(this){Orientation=Orientation.Vertical};
            var replace=new CheckBox(this){Text="Replace recorded channels",Checked=false};panel.AddView(replace);
            var countIn=new CheckBox(this){Text="One-bar count-in",Checked=true};panel.AddView(countIn);
            new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Record into app pattern")!
                .SetMessage("Capture the emitted notes, including strum and arp gates. Default: overdub. Stop saves one undoable edit. Count-in is silent; held notes carry into recording.")!
                .SetView(panel)!.SetPositiveButton("Record",(_,_)=>PerformSafely(()=> {
                    if(pendingPerformCapture is not null)throw new IOException("Previous capture is waiting for a successful save; press Stop / save to retry.");
                    connection.StartCapture((AppPattern)editing.Sequence.Current,replace.Checked?CaptureMode.ReplaceChannels:CaptureMode.Overdub,performOptions.Tempo,countIn.Checked?1:0);
                    record.Text="Recording…";if(performReadout is not null)performReadout.Text=countIn.Checked?"One silent bar count-in · then record · Stop saves":"Recording · Stop saves to Sequence";
                }))!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
        };
        var save=new Button(this){Text="Stop / save",TextSize=12};save.Click+=(_,_)=>{StopPerformance();record.Text="Record gestures";};
        recording.AddView(record,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WrapContent,1));
        recording.AddView(save,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WrapContent,1));content.AddView(recording);
        });
        Section("perform.help", "Playing tips & destination check", () => {
        content.AddView(Label($"{o.Tempo} BPM. C3 = MIDI 48; middle C is C4 (60). FM1 octave follow requires protocol 10 firmware; older firmware uses only the app's base octave.",12));
        content.AddView(Label($"1/{o.Division*4} notes\n{(o.Surface==PerformSurface.DrumPads?"Select Drums for the FM1 drum destination.":"Slide between cells; multiple fingers share sustained notes.")}\n{(o.Surface==PerformSurface.Ribbon?"Ribbon height controls velocity.":$"Pads use the configured velocity ({o.Velocity}); sliding does not change it.")}\nSwipe the ↕ rail beside the pads or joystick to scroll.\n{(o.Surface==PerformSurface.Chords?"I ↑ is the tonic one octave higher. Joystick: 7 / 9 / sus4 / sus2 / minor / major / inversions. Ninth chords omit the fifth to use four voices.":"")}",13));
        AsyncButton("Check FM1 destination transforms",async()=>{StopPerformance();performanceCheck=await connection.CheckPerformanceAsync(state.SelectedTrack,PerformChannel);ShowWorkspace();},connection.CanPerform&&!connection.Snapshot.IsGenericMidi);
        content.AddView(Label(performanceCheck,13));
        });
    }
    static string ScaleDisplay(PerformanceScale scale)=>scale switch{
        PerformanceScale.NaturalMinor=>"Natural minor",PerformanceScale.HarmonicMinor=>"Harmonic minor",
        PerformanceScale.MajorPentatonic=>"Major pentatonic",PerformanceScale.MinorPentatonic=>"Minor pentatonic",_=>scale.ToString()
    };
    void AddPerformanceGestureView(View playing,int height)
    {
        var row=new LinearLayout(this){Orientation=Orientation.Horizontal,MotionEventSplittingEnabled=true};
        row.AddView(playing,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.MatchParent,1));
        var rail=Label("↕",18);rail.Gravity=GravityFlags.Center;rail.SetTextColor(Accent);
        rail.ContentDescription="Swipe here to scroll without playing notes";
        row.AddView(rail,new LinearLayout.LayoutParams(Dp(32),ViewGroup.LayoutParams.MatchParent));
        content.AddView(row,new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MatchParent,Dp(height)));
    }
    EditProposal? pendingPerformCapture;
    void FinishPerformRecording()
    {
        if(!connection.IsRecording&&pendingPerformCapture is null)return;
        try {
            pendingPerformCapture??=connection.FinishCapture();
            if(pendingPerformCapture is {} proposal&&proposal.Changes.Length>0)editing.EditPattern(proposal);
            pendingPerformCapture=null;
            if(performReadout is not null)performReadout.Text="Recording saved · edit or undo in Sequence.";
        }
        catch(Exception error){if(performReadout is not null)performReadout.Text="Capture save failed; retained for retry: "+error.Message;}
    }
    void ChoosePerformance(string title,string[] options,int selected,Action<int> choose)
    {
        StopPerformance();new global::Android.App.AlertDialog.Builder(this)!.SetTitle(title)!.SetSingleChoiceItems(options,selected,(sender,a)=>{choose(a.Which);((global::Android.App.AlertDialog)sender!).Dismiss();ShowWorkspace();})!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
    }
    void PlayPerformance(int pointer,int cell,int touchVelocity,bool held,bool freshChord=true)
    {
        var o=performOptions;string owner=$"perform:{pointer}";
        if(!held){performFingers.Remove(pointer);if(!(o.Latch&&o.Surface==PerformSurface.Chords)){connection.Performer.Release(owner);if(performFingers.Count==0&&performReadout is not null)performReadout.Text="Released · last voicing available in touch preview.";}return;}
        if(pointer>=0)performFingers[pointer]=(cell,touchVelocity);
        int velocity=Math.Clamp(o.Velocity*touchVelocity/100,1,127);LiveNote[] notes;LiveNote? bass=null;string name;
        if(o.Surface==PerformSurface.Chords){if(o.Latch&&pointer>=0&&freshChord)latchedOctave=PerformOctave;var chord=PerformanceHarmony.Chord(o.Key,o.Scale,o.Latch?latchedOctave??PerformOctave:PerformOctave,Mapping.Degree(cell),performShape,o.Inversion,o.VoiceLead,previousVoicing);previousVoicing=chord.Notes;
            notes=chord.Notes.Select(n=>new LiveNote(PerformChannel,n)).ToArray();name=chord.Name;
            if(o.BassTrack>=0&&o.BassTrack!=state.SelectedTrack)bass=new(o.BassTrack,chord.Bass);
            if(o.Latch){owner="perform:latch";latchedDegree=cell;}
        } else {int pitch=o.Surface==PerformSurface.DrumPads?SlotBuilder.DrumLaneNote(cell):o.Surface==PerformSurface.Keyboard?Math.Clamp(12*(PerformOctave+1)+o.Key+cell,0,127):PerformanceHarmony.ScaleNote(o.Key,o.Scale,PerformOctave,cell);
            notes=[new(PerformChannel,pitch)];name=o.Surface==PerformSurface.DrumPads?$"Drum {cell+1}":PerformanceHarmony.Names[pitch%12];}
        connection.Performer.Hold(owner,notes,velocity,o.Style,o.Tempo,o.Division,o.StrumMs,bass,
            retrigger:o.Surface==PerformSurface.Chords&&o.Latch&&pointer>=0&&freshChord);
        if(performReadout is not null)performReadout.Text=$"{(connection.CanPerform?connection.Snapshot.IsGenericMidi?"MIDI":"FM1":"Offline · no audio")} · {name} · {string.Join(" ",notes.Select(n=>n.Pitch))} · velocity {velocity}{(o.Latch&&o.Surface==PerformSurface.Chords?" · LATCHED":"")}";
    }
    void PerformanceSettings()
    {
        StopPerformance();var o=performOptions;var panel=new LinearLayout(this){Orientation=Orientation.Vertical};panel.SetPadding(Dp(12),0,Dp(12),0);
        void Picker(string title,string[] values,int selected,Action<int> set){panel.AddView(Label(title,14));var spinner=new Spinner(this);spinner.Adapter=new ArrayAdapter<string>(this,global::Android.Resource.Layout.SimpleSpinnerDropDownItem,values);spinner.SetSelection(selected);spinner.ItemSelected+=(_,a)=>set(a.Position);panel.AddView(spinner);}
        void Slider(string title,int value,int min,int max,Action<int> set){var label=Label($"{title} · {value}",14);panel.AddView(label);var slider=new SeekBar(this){Max=max-min,Progress=value-min};slider.ProgressChanged+=(_,a)=>{label.Text=$"{title} · {a.Progress+min}";set(a.Progress+min);};panel.AddView(slider);}
        Picker("Key",PerformanceHarmony.Names,o.Key,v=>o=o with{Key=v});Picker("Scale",Enum.GetNames<PerformanceScale>(),(int)o.Scale,v=>o=o with{Scale=(PerformanceScale)v});
        Slider("Octave",o.Octave,1,6,v=>o=o with{Octave=v});Slider("Velocity",o.Velocity,1,127,v=>o=o with{Velocity=v});Slider("Tempo",o.Tempo,30,240,v=>o=o with{Tempo=v});
        panel.AddView(Label("C3 = MIDI 48; C4 = middle C (60). FM1 follow adds its OCT offset to this base octave.",12));
        var follow=new CheckBox(this){Text="Follow FM1 octave buttons (protocol 10)",Checked=o.FollowHardwareOctave};
        follow.CheckedChange+=(_,a)=>o=o with{FollowHardwareOctave=a.IsChecked};panel.AddView(follow);
        Picker("Rhythmic rate",["Quarter","Eighth","Sixteenth","Thirty-second"],o.Division==1?0:o.Division==2?1:o.Division==4?2:3,v=>o=o with{Division=new[]{1,2,4,8}[v]});
        Slider("Strum spacing (ms)",o.StrumMs,5,200,v=>o=o with{StrumMs=v});Slider("Inversion",o.Inversion,-3,3,v=>o=o with{Inversion=v});
        Picker("Bass destination",["Off","Synth 1","Synth 2","Synth 3"],o.BassTrack+1,v=>o=o with{BassTrack=v-1});
        if(connection.Snapshot.IsGenericMidi)Picker("MIDI channel",new[]{"Track default"}.Concat(Enumerable.Range(1,16).Select(n=>n.ToString())).ToArray(),o.ChannelOverride+1,v=>o=o with{ChannelOverride=v-1});
        else panel.AddView(Label("FM1 destination follows the selected synth/drum track.",12));
        var lead=new CheckBox(this){Text="Voice lead chords",Checked=o.VoiceLead};lead.CheckedChange+=(_,a)=>o=o with{VoiceLead=a.IsChecked};panel.AddView(lead);
        AddPerformanceControlSettings(panel,()=>o,v=>o=v);
        var scroll=new ScrollView(this);scroll.AddView(panel);
        new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Performance settings")!.SetView(scroll)!.SetPositiveButton("Apply",(_,_)=>PerformSafely(()=>{(o.Mapping??new()).Validate();(o.Macro??new()).Validate();performOptions=o;ShowWorkspace();}))!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
    }
}

sealed class PerformanceSurfaceView:View
{
    string[] labels;readonly int columns;readonly bool keyboard,ribbon;readonly Paint paint=new(){AntiAlias=true};
    public void UpdateLabels(string[] value){labels=value;Invalidate();}
    readonly Dictionary<int,int> touches=[];
    readonly HashSet<int> ignoredPointers=[];
    public event Action<int,int,int,bool>? Play;
    public event Action? Cancelled;
    public PerformanceSurfaceView(global::Android.Content.Context context,string[] labels,int columns,bool keyboard,bool ribbon):base(context){this.labels=labels;this.columns=columns;this.keyboard=keyboard;this.ribbon=ribbon;ContentDescription="Multitouch performance surface";Clickable=true;}
    public void ClearTouches(){foreach(int id in touches.Keys)ignoredPointers.Add(id);touches.Clear();Invalidate();}
    protected override void OnDraw(Canvas canvas){base.OnDraw(canvas);int rows=(labels.Length+columns-1)/columns;float w=Width/(float)columns,h=Height/(float)rows;
        for(int i=0;i<labels.Length;i++){float x=i%columns*w,y=i/columns*h;bool black=keyboard&&new[]{"C♯","E♭","F♯","A♭","B♭"}.Contains(labels[i]);
            paint.Color=touches.ContainsValue(i)?Color.Rgb(38,190,164):black?Color.Rgb(27,30,38):Color.Rgb(49,57,71);canvas.DrawRoundRect(x+3,y+3,x+w-3,y+h-3,12,12,paint);
            paint.Color=Color.White;paint.TextAlign=Paint.Align.Center;paint.TextSize=Math.Min(w*.24f,34);var lines=labels[i].Split('\n');for(int l=0;l<lines.Length;l++)canvas.DrawText(lines[l],x+w/2,y+h/2+(l-(lines.Length-1)/2f)*paint.TextSize*1.3f+paint.TextSize*.3f,paint);
        }}
    int Cell(float x,float y){int rows=(labels.Length+columns-1)/columns;if(x<0||x>=Width||y<0||y>=Height)return -1;int cell=(int)(y/Height*rows)*columns+(int)(x/Width*columns);return cell<labels.Length?cell:-1;}
    public override bool OnTouchEvent(MotionEvent? e){if(e is null)return false;
        if(e.ActionMasked==MotionEventActions.Cancel){Cancelled?.Invoke();ClearTouches();ignoredPointers.Clear();Parent?.RequestDisallowInterceptTouchEvent(false);return true;}
        if(e.ActionMasked is MotionEventActions.Up or MotionEventActions.PointerUp){int id=e.GetPointerId(e.ActionIndex);ignoredPointers.Remove(id);if(touches.Remove(id,out int old))Play?.Invoke(id,old,100,false);if(touches.Count==0)Parent?.RequestDisallowInterceptTouchEvent(false);Invalidate();PerformClick();return true;}
        if(e.ActionMasked is MotionEventActions.Down or MotionEventActions.PointerDown or MotionEventActions.Move){Parent?.RequestDisallowInterceptTouchEvent(true);
            for(int p=0;p<e.PointerCount;p++){if(e.ActionMasked!=MotionEventActions.Move&&p!=e.ActionIndex)continue;int id=e.GetPointerId(p),cell=Cell(e.GetX(p),e.GetY(p));
                if(e.ActionMasked!=MotionEventActions.Move)ignoredPointers.Remove(id);else if(ignoredPointers.Contains(id))continue;
                if(touches.TryGetValue(id,out int old)&&old==cell)continue;
                if(cell<0){if(touches.Remove(id,out old))Play?.Invoke(id,old,100,false);continue;}
                touches[id]=cell;int velocity=ribbon?(int)(127-e.GetY(p)/Height*90):100;Play?.Invoke(id,cell,velocity,true);
            }Invalidate();return true;}return true;}
    public override bool PerformClick(){base.PerformClick();return true;}
    protected override void OnDetachedFromWindow(){foreach(var pair in touches.ToArray())Play?.Invoke(pair.Key,pair.Value,100,false);ClearTouches();base.OnDetachedFromWindow();}
    protected override void Dispose(bool disposing){if(disposing)paint.Dispose();base.Dispose(disposing);}
}

sealed class ChordJoystickView:View
{
    readonly Paint paint=new(){AntiAlias=true};ChordShape current;int pointer=-1;
    public event Action<ChordShape>? Change;
    public event Action? Cancelled;
    readonly ChordShape[] sectors;
    static readonly string[] labels=["7","9","sus4","sus2","minor","major","inv −","inv +"];
    public ChordJoystickView(global::Android.Content.Context context,ChordShape[] mappings):base(context){sectors=(ChordShape[])mappings.Clone();ContentDescription="Chord quality joystick; center diatonic; clockwise from north: "+string.Join(", ",sectors);Clickable=true;}
    protected override void OnDraw(Canvas c){base.OnDraw(c);float x=Width/2f,y=Height/2f,r=Height*.38f;paint.Color=Color.Rgb(27,32,40);c.DrawCircle(x,y,r+22,paint);
        for(int i=0;i<8;i++){double a=i*Math.PI/4-Math.PI/2;paint.Color=current==sectors[i]?Color.Rgb(45,216,179):Color.LightGray;paint.TextAlign=Paint.Align.Center;paint.TextSize=27;c.DrawText(sectors[i].ToString(),x+(float)Math.Cos(a)*r,y+(float)Math.Sin(a)*r+8,paint);}
        paint.Color=current==ChordShape.Diatonic?Color.Rgb(45,216,179):Color.Gray;c.DrawCircle(x,y,r*.3f,paint);paint.Color=Color.White;paint.TextSize=22;c.DrawText("triad",x,y+7,paint);}
    public void ClearTouches(){pointer=-1;current=ChordShape.Diatonic;Invalidate();}
    void Set(ChordShape shape){if(shape==current)return;current=shape;Change?.Invoke(shape);Invalidate();}
    public override bool OnTouchEvent(MotionEvent? e){if(e is null)return false;
        if(e.ActionMasked==MotionEventActions.Down){pointer=e.GetPointerId(0);Parent?.RequestDisallowInterceptTouchEvent(true);}
        if(e.ActionMasked==MotionEventActions.Cancel||e.ActionMasked==MotionEventActions.Up||e.ActionMasked==MotionEventActions.PointerUp&&e.GetPointerId(e.ActionIndex)==pointer){if(e.ActionMasked==MotionEventActions.Cancel)Cancelled?.Invoke();pointer=-1;Set(ChordShape.Diatonic);Parent?.RequestDisallowInterceptTouchEvent(false);return true;}
        int index=e.FindPointerIndex(pointer);if(index<0)return true;float dx=e.GetX(index)-Width/2f,dy=e.GetY(index)-Height/2f,r=Height*.38f;double distance=Math.Sqrt(dx*dx+dy*dy);
        if(distance<r*(current==ChordShape.Diatonic?.40:.28)){Set(ChordShape.Diatonic);return true;}
        double angle=(Math.Atan2(dy,dx)+Math.PI/2+Math.PI*2)%(Math.PI*2),sector=angle/(Math.PI/4);int selected=(int)Math.Floor(sector+.5)%8;
        int old=Array.IndexOf(sectors,current);if(old>=0){double delta=Math.Abs(sector-old);delta=Math.Min(delta,8-delta);if(delta<.65)return true;}
        Set(sectors[selected]);return true;}
    protected override void OnDetachedFromWindow(){pointer=-1;Set(ChordShape.Diatonic);base.OnDetachedFromWindow();}
    protected override void Dispose(bool disposing){if(disposing)paint.Dispose();base.Dispose(disposing);}
}



