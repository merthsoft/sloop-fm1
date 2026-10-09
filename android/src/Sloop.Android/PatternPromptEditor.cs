using Android.Widget;

using System.Collections.Immutable;

using Sloop.Sequencing;

using Sloop.Workstation;



namespace Sloop.Android;

public sealed partial class MainActivity

{

    void DescribePatternCommands(AppPattern pattern,int channel)

    {

        var panel=new LinearLayout(this){Orientation=Orientation.Vertical};

        var compose=new Button(this){Text="Compose a new offline loop…"};

        compose.Click+=(_,_)=>DescribeOfflineComposition(pattern);panel.AddView(compose);

        var saved=new Button(this){Text="Open saved composition drafts…"};saved.Click+=(_,_)=>OpenCompositionDrafts(pattern);panel.AddView(saved);

        panel.AddView(Label($"Channel {channel+1} · offline commands edit existing notes. Other channels stay unchanged.",14));

        var scopes=new Spinner(this){Adapter=new ArrayAdapter<string>(this,global::Android.Resource.Layout.SimpleSpinnerDropDownItem,new[]{"Whole channel","Selected bar","Selected step"})};panel.AddView(scopes);

        long sixteenth=Math.Max(1,pattern.TicksPerQuarter/4),onset=editing.Step*sixteenth;

        var preserve=new CheckBox(this){Text="Lock notes at selected step (conflicting edits reject)"};panel.AddView(preserve);

        var phrases=new[]{"transpose up 1 semitones","transpose down 12 semitones",$"quantize {sixteenth} ticks","velocity up 10","velocity down 10",$"move later {sixteenth} ticks",$"resize longer {sixteenth} ticks","simplify"};

        var prompt=new EditText(this){Text=phrases[0],Hint="One supported command"};

        var choices=new Spinner(this){Adapter=new ArrayAdapter<string>(this,global::Android.Resource.Layout.SimpleSpinnerDropDownItem,phrases)};

        choices.ItemSelected+=(_,a)=>prompt.Text=phrases[a.Position];panel.AddView(choices);panel.AddView(prompt);

        panel.AddView(Label(PromptEditor.AppCommands+"\nSimplify keeps approximately every second selected note with a fixed seed. Additional clauses reject.",12));

        StyleContent(panel);

        new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Edit sequence through prompts")!.SetView(panel)!

            .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Propose",(_,_)=> {

                try{

                    long from=scopes.SelectedItemPosition==1?editing.Step/16*16*sixteenth:onset;

                    long to=scopes.SelectedItemPosition==1?from+16*sixteenth:from+sixteenth;

                    var notes=pattern.Notes.Where(n=>n.Channel==channel&&(scopes.SelectedItemPosition==0||n.Start.Value>=from&&n.Start.Value<to));

                    var selection=new AppSelection(notes.Select(n=>n.Id).ToImmutableHashSet());

                    var locks=pianoRoll.Locks;

                    if(preserve.Checked)locks=locks with {Events=locks.Events.Union(pattern.Notes.Where(n=>n.Channel==channel&&n.Start.Value==onset).Select(n=>n.Id))};

                    ReviewPatternProposal(PromptEditor.Propose(pattern,selection,prompt.Text??"",locks,42),false);

                }catch(Exception e){editing.SetStatus(e.Message);}

            })!.Show();

    }



    void DescribeOfflineComposition(AppPattern source)

    {

        var panel=new LinearLayout(this){Orientation=Orientation.Vertical};

        panel.AddView(Label("Procedural composition · editable local draft. Bass: MIDI 1; chords: 2; melody: 3; drums: 10. Apply replaces only requested channels and sets loop length. Other notes must fit the new length.",14));

        var prompt=new EditText(this){Text="key=C scale=minor bars=4 seed=42 parts=bass,chords,melody,drums density=steady",Hint="Supported fields only"};panel.AddView(prompt);

        panel.AddView(Label(OfflineComposition.Vocabulary,12));

        panel.AddView(Label("Controls below override optional prompt fields.",12));

        var tempo=new EditText(this){Text=Math.Clamp(editing.Tempo,40,240).ToString(System.Globalization.CultureInfo.InvariantCulture),Hint="Draft tempo 40..240 BPM"};panel.AddView(tempo);

        Spinner Choice(string label,string[] values){panel.AddView(Label(label,12));var choice=new Spinner(this){Adapter=new ArrayAdapter<string>(this,global::Android.Resource.Layout.SimpleSpinnerDropDownItem,values)};panel.AddView(choice);return choice;}

        string[] progressions=["classic","pop","minor-turnaround"],rhythms=["straight","offbeat","swing"];

        var progression=Choice("Progression: I–IV–V–I / I–V–vi–IV / i–VI–iv–V",progressions);var rhythm=Choice("Rhythm",rhythms);StyleContent(panel);

        var scroll=new ScrollView(this);scroll.AddView(panel);

        new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Compose offline loop")!.SetView(scroll)!

            .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Generate draft",async (_,_)=> {

                try{var text=string.Join(" ",(prompt.Text??"").Split((char[]?)null,StringSplitOptions.RemoveEmptyEntries).Where(f=>!new[]{"tempo","progression","rhythm"}.Contains(f.Split('=')[0].ToLowerInvariant())))+$" tempo={tempo.Text} progression={progressions[progression.SelectedItemPosition]} rhythm={rhythms[rhythm.SelectedItemPosition]}";var draft=await Task.Run(()=>OfflineComposition.Generate(text,source.TicksPerQuarter));ReviewCompositionDraft(source,draft);}

                catch(Exception e){editing.SetStatus(e.Message);}

            })!.Show();

    }



    void ReviewCompositionDraft(AppPattern source,CompositionDraft draft,ImmutableHashSet<CompositionPart>? kept=null)

    {

        var panel=new LinearLayout(this){Orientation=Orientation.Vertical};

        panel.AddView(Label($"Seed {draft.Intent.Seed} · {PerformanceHarmony.Names[draft.Intent.Key]} {draft.Intent.Scale} · {draft.Intent.Bars} bars · {draft.Intent.Density}\n{draft.Pattern.Notes.Length} editable notes. Apply is one local undo step; normal hardware Send stays separate.",14));

        var keep=new Dictionary<CompositionPart,CheckBox>();

        foreach(var part in draft.Intent.Parts){var check=new CheckBox(this){Text=$"Keep {part} (including draft edits) on regeneration",Checked=kept?.Contains(part)??false};keep.Add(part,check);panel.AddView(check);}

        var notes=draft.Pattern.Notes.OrderBy(n=>n.Channel).ThenBy(n=>n.Start.Value).ThenBy(n=>n.Pitch).ToArray();

        var selector=new Spinner(this){Adapter=new ArrayAdapter<string>(this,global::Android.Resource.Layout.SimpleSpinnerDropDownItem,

            notes.Select(n=>$"Ch {n.Channel+1}: pitch {n.Pitch}, tick {n.Start.Value}, duration {n.Duration.Value}, velocity {n.Velocity}").ToArray())};panel.AddView(selector);

        var edit=new Button(this){Text="Edit selected draft note",Enabled=notes.Length>0};panel.AddView(edit);

        var builder=new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Review composition draft")!.SetNegativeButton("Discard",(_,_)=>{})!;

        var scroll=new ScrollView(this);scroll.AddView(panel);StyleContent(panel);builder.SetView(scroll);

        builder.SetNeutralButton("Regenerate",async (_,_)=> {

            var preserved=keep.Where(p=>p.Value.Checked).Select(p=>p.Key).ToImmutableHashSet();

            try{var next=await Task.Run(()=>OfflineComposition.Regenerate(draft,preserved));ReviewCompositionDraft(source,next,preserved);}

            catch(Exception e){editing.SetStatus(e.Message);ReviewCompositionDraft(source,draft,preserved);}

        });

        builder.SetPositiveButton("Review apply",(_,_)=> {

            try{ReviewPatternProposal(OfflineComposition.ProposeApply(source,draft,pianoRoll.Locks),false);}

            catch(Exception e){editing.SetStatus(e.Message);ReviewCompositionDraft(source,draft,keep.Where(p=>p.Value.Checked).Select(p=>p.Key).ToImmutableHashSet());}

        });

        var dialog=builder.Create()!;

        var audition=new Button(this){Text="Audition draft through MIDI",Enabled=connection.CanPerform && !connection.IsPlaying && draft.Pattern.Notes.Length>0};panel.AddView(audition);

        var stop=new Button(this){Text="Stop draft audition"};panel.AddView(stop);stop.Click+=(_,_)=>StopCompositionAudition();

        audition.Click+=async (_,_)=>{audition.Enabled=false;try{await AuditionCompositionDraft(draft);}catch(Exception e){editing.SetStatus(e.Message);}finally{if(!IsDestroyed&&!IsFinishing)audition.Enabled=connection.CanPerform&&!connection.IsPlaying;}};

        var save=new Button(this){Text="Save named draft…"};panel.AddView(save);save.Click+=(_,_)=>SaveCompositionDraft(draft,keep.Where(p=>p.Value.Checked).Select(p=>p.Key).ToImmutableArray());

        var export=new Button(this){Text="Export portable draft…"};panel.AddView(export);export.Click+=(_,_)=>ExportCompositionArchive(draft,keep.Where(p=>p.Value.Checked).Select(p=>p.Key).ToImmutableArray());

        dialog.DismissEvent+=(_,_)=>StopCompositionAudition();

        edit.Click+=(_,_)=> {

            if(selector.SelectedItemPosition<0 || selector.SelectedItemPosition>=notes.Length)return;

            dialog.Dismiss();EditCompositionNote(source,draft,notes[selector.SelectedItemPosition],keep.Where(p=>p.Value.Checked).Select(p=>p.Key).ToImmutableHashSet());

        };

        dialog.Show();

    }



    void EditCompositionNote(AppPattern source,CompositionDraft draft,AppNote note,ImmutableHashSet<CompositionPart> kept)

    {

        var panel=new LinearLayout(this){Orientation=Orientation.Vertical};

        EditText Field(string name,long value){panel.AddView(Label(name,12));var input=new EditText(this){Text=value.ToString(System.Globalization.CultureInfo.InvariantCulture)};panel.AddView(input);return input;}

        var pitch=Field("MIDI pitch 0..127",note.Pitch);var velocity=Field("Velocity 1..127",note.Velocity);

        var start=Field("Start tick",note.Start.Value);var duration=Field("Duration ticks",note.Duration.Value);StyleContent(panel);

        var scroll=new ScrollView(this);scroll.AddView(panel);

        new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Edit draft note")!.SetView(scroll)!

            .SetNegativeButton("Back",(_,_)=>ReviewCompositionDraft(source,draft,kept))!

            .SetNeutralButton("Delete note",(_,_)=> {

                try{ReviewCompositionDraft(source,OfflineComposition.EditNote(draft,note.Id,null),kept);}

                catch(Exception e){editing.SetStatus(e.Message);ReviewCompositionDraft(source,draft,kept);}

            })!.SetPositiveButton("Update draft",(_,_)=> {

                try{

                    var updated=note with {Pitch=int.Parse(pitch.Text??"",System.Globalization.CultureInfo.InvariantCulture),Velocity=int.Parse(velocity.Text??"",System.Globalization.CultureInfo.InvariantCulture),Start=new(long.Parse(start.Text??"",System.Globalization.CultureInfo.InvariantCulture)),Duration=new(long.Parse(duration.Text??"",System.Globalization.CultureInfo.InvariantCulture))};

                    ReviewCompositionDraft(source,OfflineComposition.EditNote(draft,note.Id,updated),kept);

                }catch(Exception e){editing.SetStatus(e.Message);ReviewCompositionDraft(source,draft,kept);}

            })!.Show();

    }



    void DescribeNativeCommands(HardwarePattern pattern,HardwareSelection selection)

    {

        var panel=new LinearLayout(this){Orientation=Orientation.Vertical};var track=pattern.Tracks.Single(t=>t.Id==selection.TrackId);

        panel.AddView(Label($"Native selected step {selection.Range.Start.Value+1} · {(track.IsDrum?"all drum lanes":"synth track")} · local edits only",14));

        var prompt=new EditText(this){Text=track.IsDrum?"level ghost":"transpose up 1 semitones",Hint="One native command"};panel.AddView(prompt);

        panel.AddView(Label(PromptEditor.HardwareCommands+"\nTiming belongs to the entire step; note durations cannot be resized. Simplify supports drums only. Hardware Send stays separate.",12));StyleContent(panel);

        new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Offline native pattern commands")!.SetView(panel)!

            .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Propose",(_,_)=> {

                try{ReviewPatternProposal(PromptEditor.Propose(pattern,selection,prompt.Text??"",seed:42),true);}catch(Exception e){editing.SetStatus(e.Message);}

            })!.Show();

    }



    void ReviewPatternProposal(EditProposal proposal,bool native)

    {

        string Note(AppNote? n)=>n is null?"none":$"channel {n.Channel+1}, pitch {n.Pitch}, tick {n.Start.Value}, length {n.Duration.Value}, velocity {n.Velocity}";

        var lines=new List<string>{proposal.Provenance.Description};

        if(proposal.Before is AppPattern oldPattern && proposal.After is AppPattern newPattern && oldPattern.Length!=newPattern.Length)

            lines.Add($"Loop length: {oldPattern.Length.Value} → {newPattern.Length.Value} ticks");

        foreach(var change in proposal.Changes){

            if(change is NoteChange note)lines.Add(Note(note.Before)+" → "+Note(note.After));

            else if(change is TrackChange track){

                string Step(HardwareStep step)=>$"micro {step.Micro.Value}/64, {step.Fill}; "+(step switch{

                    SynthStep synth=>$"{synth.Time}, {synth.Flags}, velocity {synth.Velocity}, {synth.Count} active notes; "+

                        string.Join("; ",synth.Slots.Select((slot,i)=>$"note {i+1}: {slot.Pitch}, {slot.Level}, ratchet {slot.Ratchet}")),

                    DrumStep drum=>string.Join("; ",drum.Lanes.Select((hit,i)=>$"lane {i+1}: {(hit.On?"on":"off")}, {hit.Level}, ratchet {hit.Ratchet}")),

                    _=>"Unknown step"

                });

                for(int i=0;i<track.Before.Steps.Length;i++){

                    var before=Step(track.Before.Steps[i]);var after=Step(track.After.Steps[i]);

                    if(before!=after)lines.Add($"Step {i+1}:\n{before}\n→ {after}");

                }

                string Locks(IEnumerable<ParameterLock> locks)=>string.Join("; ",locks.Select(l=>$"step {l.Step.Value+1}, parameter {l.Parameter}: {l.Value}"));

                if(!track.Before.Locks.SequenceEqual(track.After.Locks))lines.Add("Parameter locks: "+Locks(track.Before.Locks)+" → "+Locks(track.After.Locks));

            }

        }

        if(proposal.Changes.IsEmpty)lines.Add("No changes; nothing to apply.");

        var review=new ScrollView(this);review.AddView(Label(string.Join("\n\n",lines),14));

        var builder=new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Review proposed changes")!.SetView(review)!.SetNegativeButton("Discard",(_,_)=>{})!;

        if(!proposal.Changes.IsEmpty)builder.SetPositiveButton("Apply locally",(_,_)=>editing.Run(()=> {

            if(native)editing.EditNative(proposal);else editing.EditPattern(proposal);

        },"Prompt edit applied. Local Undo is available."));

        builder.Show();

    }

}

