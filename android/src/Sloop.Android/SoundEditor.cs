using Android.Widget;
using Android.Views;
using Sloop.SoundDesign;
using Sloop.Sequencing;
using Sloop.Android.Services;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    private EditingWorkspace editing=null!;
    private readonly SoundState?[] hardwareSound=new SoundState?[3];
    private bool soundPromptPage;
    private readonly SoundAuditionRetention soundAuditions=SoundAuditionRetention.Current;
    private static Fm1Connection? auditionConnection;
    private void InitializeSoundAuditions()
    {
        if(!ReferenceEquals(auditionConnection,connection)) {
            if(auditionConnection is not null) {
                auditionConnection.Changed-=OnRetainedAuditionConnectionChanged;
                soundAuditions.NotifyDisconnected();
            }
            auditionConnection=connection;
            auditionConnection.Changed+=OnRetainedAuditionConnectionChanged;
        }
        OnRetainedAuditionConnectionChanged();
    }
    private static void OnRetainedAuditionConnectionChanged()
    {
        if(auditionConnection?.Snapshot.Device is null||auditionConnection.Snapshot.IsSimulated||auditionConnection.Snapshot.IsGenericMidi)
            SoundAuditionRetention.Current.NotifyDisconnected();
    }
    private void OnSoundAuditionWorkspaceReplaced()=>soundAuditions.NotifyWorkspaceReplaced();
    private void OnEditingChanged() {ObserveSceneWorkspace();if(state.Workspace is Sloop.Core.Workspace.Sound or Sloop.Core.Workspace.Sequence or Sloop.Core.Workspace.Library)ShowWorkspace();}
    private void ActionButton(string title,Action action,bool enabled=true)
    {
        var b=new Button(this){Text=title,Enabled=enabled}; b.Click+=(_,_)=>editing.Run(action,"Local edit saved."); content.AddView(b);
    }
    private void AsyncButton(string title,Func<Task> action,bool enabled=true)
    {
        var b=new Button(this){Text=title,Enabled=enabled}; b.Click+=async(_,_)=> {
            try {await action();}catch(Exception e){editing.SetStatus(e.Message);}
        }; content.AddView(b);
    }
    private void NumberControl(string label,int value,int min,int max,Action<int> set)
    {
        var text=Label($"{label} · {value}",14); content.AddView(text);
        text.Clickable=true; text.Focusable=true; text.ContentDescription=$"{label}, {value}. Tap to enter a value from {min} to {max}.";
        text.Click+=(_,_)=> {
            var entry=new EditText(this){Text=value.ToString(),InputType=global::Android.Text.InputTypes.ClassNumber|global::Android.Text.InputTypes.NumberFlagSigned};
            new global::Android.App.AlertDialog.Builder(this)!.SetTitle($"{label} ({min}–{max})")!.SetView(entry)!
                .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Save",(_,_)=>editing.Run(()=> {
                    if(!int.TryParse(entry.Text,out int number)||number<min||number>max)throw new ArgumentException($"Enter a whole number from {min} to {max}.");
                    set(number);
                },"Local edit saved."))!.Show();
        };
        var slider=new SeekBar(this){Max=max-min,Progress=value-min,ContentDescription=label};
        slider.ProgressChanged+=(_,a)=>text.Text=$"{label} · {a.Progress+min}";
        slider.StopTrackingTouch+=(_,_)=>editing.Run(()=>set(slider.Progress+min),"Local edit saved.");
        content.AddView(slider);
    }
    private void AddSoundEditor()
    {
        int track=state.SelectedTrack;
        content.AddView(Label(editing.Status,14));
        for(int recoveryTrack=0;recoveryTrack<3;recoveryTrack++)if(soundAuditions[recoveryTrack]?.Active==true) {
            int selected=recoveryTrack;
            var retained=soundAuditions[selected]!;
            content.AddView(Label($"Retained FM1 audition · Synth {selected+1} · {retained.Status}"+
                (soundAuditions.WorkspaceReplaced(selected)?" · session replaced; restore or abandon before a new B":""),14));
            if(track!=selected)ButtonRow(("Recover Synth "+(selected+1)+" audition",()=>{state.SelectTrack(selected);ShowWorkspace();},true));
        }
        if(track==3){content.AddView(Label("FM6 patches belong to the three synth tracks.",14));return;}
        var doc=editing.Sounds[track]; var patch=doc.State.Patch;
        content.AddView(Label($"Local FM6 patch · {patch.Name}",22));
        var pages=new LinearLayout(this){Orientation=Orientation.Horizontal};
        foreach(var item in new[]{(Title:"Edit sound",Prompt:false),(Title:"Describe a sound",Prompt:true)}) {
            var button=new Button(this){Text=item.Title,Activated=soundPromptPage==item.Prompt};
            button.Click+=(_,_)=>{soundPromptPage=item.Prompt;ShowWorkspace();content.Post(()=> (content.Parent as ScrollView)?.ScrollTo(0,0));};
            pages.AddView(button,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WrapContent,1));
        }
        content.AddView(pages);
        content.AddView(Label(soundPromptPage?"Offline sound recipes · describe changes, review the proposal, then apply locally.":"Edits save locally. Read/Apply exchanges the whole patch with FM1.",14));
        if(soundPromptPage) Section("sound.prompt", "Describe a sound - offline prompts", () => {
        var prompt=new EditText(this){Text=editing.Prompt,Hint="Describe changes · offline recipes"}; prompt.TextChanged+=(_,_)=>editing.Prompt=prompt.Text??"";content.AddView(prompt);
        ActionButton($"Family · {editing.Family}",()=>new global::Android.App.AlertDialog.Builder(this)!.SetItems(Enum.GetNames<SoundFamily>(),(_,a)=>{editing.Family=Enum.GetValues<SoundFamily>()[a.Which];ShowWorkspace();})!.Show());
        Section("sound.prompt.locks","Keep parts of the current sound",()=> {
        foreach(var group in new[]{LockGroup.Algorithm,LockGroup.Tuning,LockGroup.Envelopes,LockGroup.Attack,LockGroup.VoiceControls}) {
            var c=new CheckBox(this){Text=$"Keep {group}",Checked=editing.Locks.Groups.HasFlag(group)}; c.SetTextColor(global::Android.Graphics.Color.White);
            c.CheckedChange+=(_,a)=>editing.Locks=editing.Locks with{Groups=a.IsChecked?editing.Locks.Groups|group:editing.Locks.Groups&~group};content.AddView(c);
        }
        });
        ActionButton("Propose changes to current patch",()=> {
            var parsed=PhraseParser.Parse(editing.Prompt,editing.Family,42);
            if(!parsed.HasSupportedRequest)throw new ArgumentException(string.Join("\n",parsed.Notices));
            var locks=new PatchLocks(editing.Locks.Groups|parsed.Locks.Groups,editing.Locks.Operators.Union(parsed.Locks.Operators));
            var proposal=ProceduralDesigner.Refine(doc.TargetId,doc.Revision,doc.State,parsed.Intent,parsed.Refinements,locks,editing.Prompt);
            editing.Drafts[track]=proposal with{Explanation=proposal.Explanation.AddRange(parsed.Notices)};
        });
        ActionButton("Create four sound variations",()=> {
            var parsed=PhraseParser.Parse(editing.Prompt,editing.Family,42);
            if(!parsed.HasSupportedRequest)throw new ArgumentException(string.Join("\n",parsed.Notices));
            var locks=new PatchLocks(editing.Locks.Groups|parsed.Locks.Groups,editing.Locks.Operators.Union(parsed.Locks.Operators));
            var drafts=ProceduralDesigner.Generate(doc.TargetId,doc.Revision,doc.State,parsed.Intent,locks,editing.Prompt)
                .Select(d=>d with{Explanation=d.Explanation.AddRange(parsed.Notices)}).ToArray();
            new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Choose a local proposal")!
                .SetItems(drafts.Select(d=>$"{d.FullName} · {d.Changes.Length} fields").ToArray(),(_,a)=>{editing.Drafts[track]=drafts[a.Which];ShowWorkspace();})!.Show();
        });
        if(editing.Drafts[track] is {} draft) {
            content.AddView(Label("Proposal · "+string.Join("\n",draft.Explanation),14));
            content.AddView(Label(string.Join("\n",draft.Changes.Take(30).Select(c=>$"{c.Field}: {c.Before} → {c.After}"))+$"\n{draft.Changes.Length} changed fields. Macros: {draft.After.Macros}",14));
            ActionButton("Apply proposal locally",()=>editing.ApplyDraft(track));
            ActionButton("Discard proposal",()=>editing.Drafts[track]=null);
        }
        },true);
        AddSoundAudition(track,doc);
        if(soundPromptPage)return;
        Section("sound.operators", "FM6 operators", () => {
        int effectiveAlgorithm=doc.State.Macros.AlgorithmOverride is >0 ? doc.State.Macros.AlgorithmOverride : patch.Algorithm;
        NumberControl("Patch algorithm",patch.Algorithm,1,32,value=> {
            var current=editing.Sounds[track].State;
            editing.ChangeSound(track,current with{Patch=current.Patch with{Algorithm=value}},"Change algorithm");
        });
        if(doc.State.Macros.AlgorithmOverride>0)content.AddView(Label($"Track macro overrides the patch: diagram shows algorithm {effectiveAlgorithm}.",14));
        content.AddView(new AlgorithmDiagram(this,effectiveAlgorithm,editing.Operator,chosen=>{editing.Operator=chosen;ShowWorkspace();}),
            new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MatchParent,Dp(220)));
        content.AddView(Label("Teal = audible carrier · amber loop = feedback · tap an operator",12));
        var operatorButtons=new LinearLayout(this){Orientation=Orientation.Horizontal};
        for(int op=1;op<=6;op++){int chosen=op;var b=new Button(this){Text=$"{op}",Activated=op==editing.Operator}; b.Click+=(_,_)=>{editing.Operator=chosen;ShowWorkspace();};operatorButtons.AddView(b,new LinearLayout.LayoutParams(0,ViewGroup.LayoutParams.WrapContent,1));}content.AddView(operatorButtons);
        int selected=editing.Operator; content.AddView(Label($"Operator {selected}",22));
        var carrier=Sloop.SoundDesign.AlgorithmTopology.IsCarrier(effectiveAlgorithm,selected);
        content.AddView(Label($"Algorithm {effectiveAlgorithm} · {(carrier?"carrier":"modulator")} · frequency {(patch.GetOperator(selected).Mode==FrequencyMode.Ratio?"ratio":"fixed")}",14));
        void OperatorTool(bool swap) {
            int[] targets=Enumerable.Range(1,6).Where(n=>n!=selected).ToArray();
            new global::Android.App.AlertDialog.Builder(this)!.SetTitle(swap?$"Swap OP{selected} with…":$"Copy OP{selected} to…")!
                .SetItems(targets.Select(n=>$"Operator {n}").ToArray(),(_,a)=>editing.Run(()=> {
                    var current=editing.Sounds[track].State;
                    var after=swap?OperatorEditing.Swap(current.Patch,selected,targets[a.Which]):OperatorEditing.Copy(current.Patch,selected,targets[a.Which]);
                    editing.ChangeSound(track,current with{Patch=after},swap?"Swap operators":"Copy operator");
                },"Operator edit saved. Undo restores the previous patch."))!.SetNegativeButton("Cancel",(_,_)=>{})!.Show();
        }
        ButtonRow(("Copy to…",()=>OperatorTool(false),true),("Swap with…",()=>OperatorTool(true),true));
        var opLock=new CheckBox(this){Text=$"Keep operator {selected} in proposals",Checked=editing.Locks.Operators.Contains(selected)};opLock.SetTextColor(global::Android.Graphics.Color.White);
        opLock.CheckedChange+=(_,a)=>editing.Locks=editing.Locks with{Operators=a.IsChecked?editing.Locks.Operators.Add(selected):editing.Locks.Operators.Remove(selected)};content.AddView(opLock);
        string[] fields=["Rate 1","Rate 2","Rate 3","Rate 4","Level 1","Level 2","Level 3","Level 4","Key breakpoint","Left depth","Right depth","Left curve","Right curve","Rate scaling","Amplitude sensitivity","Velocity sensitivity","Output level","Frequency mode (0 ratio, 1 fixed)","Coarse","Fine","Detune"];
        int[] bounds=[99,99,99,99,99,99,99,99,99,99,99,3,3,7,3,7,99,1,31,99,14];
        void Fields(IEnumerable<int> selectedFields) { foreach(int f in selectedFields){int index=(6-selected)*21+f;var bytes=PatchCodec.EncodeVoice(patch);
            NumberControl(fields[f],bytes[index],0,bounds[f],value=> {var current=PatchCodec.EncodeVoice(doc.State.Patch);current[index]=(byte)value;editing.ChangeSound(track,doc.State with{Patch=PatchCodec.DecodeVoice(current)},"Manual operator edit");});}
        }
        Section("sound.tone", "Tone · level & frequency", () => Fields(Enumerable.Range(16,5)), true);
        Section("sound.envelope", "Envelope · rates & levels", () => {
            content.AddView(new OperatorEnvelopeView(this,patch.GetOperator(selected).Envelope,(stage,level)=>editing.Run(()=> {
                var current=editing.Sounds[track].State;
                editing.ChangeSound(track,current with{Patch=OperatorEditing.SetEnvelopeLevel(current.Patch,selected,stage,level)},"Envelope level drag");
            },"Envelope level saved.")),new LinearLayout.LayoutParams(ViewGroup.LayoutParams.MatchParent,Dp(170)));
            content.AddView(Label("Drag levels vertically. Stage spacing is schematic; rates are native 0–99 values. Tap any numeric label for precise entry.",12));
            Fields(Enumerable.Range(0,8));
        },true);
        Section("sound.scaling", "Keyboard & velocity response", () => Fields(Enumerable.Range(8,8)));
        }, true);
        Section("sound.globals", "Global voice controls", () => {
        string[] globals=["Pitch rate 1","Pitch rate 2","Pitch rate 3","Pitch rate 4","Pitch level 1","Pitch level 2","Pitch level 3","Pitch level 4","Algorithm","Feedback","Oscillator sync","LFO speed","LFO delay","LFO pitch depth","LFO amplitude depth","LFO sync","LFO waveform","Pitch sensitivity","Transpose (24 neutral)"];
        int[] maxes=[99,99,99,99,99,99,99,99,31,7,1,99,99,99,99,1,5,7,48];
        for(int f=0;f<19;f++){int index=126+f; int offset=index==134?1:0;var bytes=PatchCodec.EncodeVoice(patch);
            NumberControl(globals[f],bytes[index]+offset,offset,maxes[f]+offset,value=>{var current=PatchCodec.EncodeVoice(doc.State.Patch);current[index]=(byte)(value-offset);editing.ChangeSound(track,doc.State with{Patch=PatchCodec.DecodeVoice(current)},"Manual global edit");});}
        });
        Section("sound.macros", "Track sound macros", () => {
        var m=doc.State.Macros; int[] macroValues=[m.AlgorithmOverride,m.Feedback,m.ModulatorLevel,m.ModulatorRatio,m.ModulatorEnvelope,m.VelocityModulation,m.Detune];
        string[] macroNames=["Algorithm override (0 = patch)","Feedback offset","Modulator level","Modulator ratio","Modulator envelope","Velocity modulation","Track detune"];
        int[] minimum=[0,-7,-64,-16,-64,-7,0], maximum=[32,7,63,16,63,7,127];
        for(int i=0;i<7;i++){int field=i;NumberControl(macroNames[i],macroValues[i],minimum[i],maximum[i],v=> {
            var current=doc.State.Macros;current=field switch{0=>current with{AlgorithmOverride=v},1=>current with{Feedback=v},2=>current with{ModulatorLevel=v},3=>current with{ModulatorRatio=v},4=>current with{ModulatorEnvelope=v},5=>current with{VelocityModulation=v},_=>current with{Detune=v}};
            editing.ChangeSound(track,doc.State with{Macros=current},"Manual macro edit");
        });}
        });
        if(!soundPromptPage) Section("sound.files", "Patch name, templates & files", () => {
        var name=new EditText(this){Text=patch.Name,Hint="Device patch name (10 ASCII characters)"};content.AddView(name);
        ActionButton("Save patch name",()=>editing.ChangeSound(track,doc.State with{Patch=doc.State.Patch with{Name=name.Text??""}},"Rename"));
        ActionButton("Factory template",()=>new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Local factory patch")!
            .SetItems(Enumerable.Range(0,8).Select(i=>FactoryLibrary.Get(i).Name).ToArray(),(_,a)=>editing.Run(()=>editing.ChangeSound(track,new(FactoryLibrary.Get(a.Which),doc.State.Macros),"Factory patch"),"Factory patch saved locally."))!.Show());
        ActionButton("Undo local sound",()=>editing.SoundHistory(track,false));
        ActionButton("Redo local sound",()=>editing.SoundHistory(track,true));
        ActionButton("Import DX voice or bank SysEx",()=>ImportSound(track));
        ActionButton("Export current voice SysEx",()=>ExportBytes(SysExCodec.ExportSingle(doc.State.Patch),"sloop-voice.syx"));
        });
        AddSoundBankEditor(track,doc);
        Section("sound.device", "FM1 - read, apply & audition", () => {
        AddLevelControl();
        bool linked=!connection.Snapshot.Busy&&!connection.IsPlaying&&!connection.Snapshot.IsSimulated&&connection.Snapshot.Device?.ProtocolVersion>=9;
        AsyncButton("Read FM6 sound from FM1",async()=> {
            var value=await connection.ReadSoundAsync(track);editing.ChangeSound(track,value,"Device read");hardwareSound[track]=value;
            editing.SetStatus("FM6 patch and macros read; local snapshot saved.");
        },linked);
        AsyncButton("Apply local sound to FM1 RAM",async()=> {
            var desired=doc.State;
            hardwareSound[track]=await connection.ApplySoundAsync(track,hardwareSound[track]??throw new IOException("Read this track first."),desired);
            editing.SetStatus("FM6 RAM patch and macros verified. Save to a hardware bank separately for project persistence.");
        },linked&&hardwareSound[track] is not null&&soundAuditions[track]?.Active!=true);
        AsyncButton("Audition current FM1 sound",async()=> {
            var plan=ProceduralDesigner.Refine(doc.TargetId,doc.Revision,doc.State,new(editing.Family,42),[]).Audition;
            var part=((AppPattern)editing.Sequence.Current).Id;
            await connection.PlayNotesAsync(plan.Notes.Select(n=>new AppNote(Guid.NewGuid(),part,new(n.StartStep*240),new(n.LengthSteps*240),n.MidiNote,n.Velocity,track)),960,plan.Tempo);
        },linked);
        ActionButton("Stop MIDI preview",()=>connection.StopPlaying());
        });
    }
    private void AddSoundAudition(int track,SoundDocument doc)
    {
        Section("sound.ab","FM1 A/B · reversible RAM audition",()=> {
            var audition=soundAuditions[track];
            bool linked=!connection.Snapshot.Busy&&!connection.IsPlaying&&!connection.Snapshot.IsSimulated&&connection.Snapshot.Device?.ProtocolVersion>=9;
            async Task Run(Func<Task> action) {try{await action();}finally{editing.SetStatus(soundAuditions[track]?.Message??"Audition ended.");ShowWorkspace();}}
            content.AddView(Label(audition?.Message??"Capture acknowledged original A, then compare the local patch B. Local saving does not keep device RAM or write a bank.",14));
            AsyncButton("Capture original A from FM1",async()=> {
                var device=connection;
                var captured=await device.ReadSceneSoundsAsync([track]);
                soundAuditions.Capture(track,new(captured,doc.TargetId,doc.Revision,
                    ()=>device.ReadSceneSoundsAsync([track]),
                    (baseline,desired)=>device.ApplySceneSoundsAsync(baseline,[new(track,desired)]),device.StopPlaying));
                editing.SetStatus(soundAuditions[track]!.Message);ShowWorkspace();
            },linked&&audition?.Active!=true);
            if(audition?.Active!=true)return;
            bool ready=linked&&!audition.Busy;
            bool safe=ready&&audition.Status is not (SoundAuditionStatus.Unknown or SoundAuditionStatus.Conflict);
            AsyncButton("Play phrase on verified A/B",async()=> {
                var current=editing.Sounds[track];
                var plan=ProceduralDesigner.Refine(current.TargetId,current.Revision,current.State,new(editing.Family,42),[]).Audition;
                var part=((AppPattern)editing.Sequence.Current).Id;
                await connection.PlayNotesAsync(plan.Notes.Select(n=>new AppNote(Guid.NewGuid(),part,new(n.StartStep*240),new(n.LengthSteps*240),n.MidiNote,n.Velocity,track)),960,plan.Tempo);
            },ready&&audition.Status is SoundAuditionStatus.Original or SoundAuditionStatus.Candidate);
            ActionButton("Stop audition phrase",()=>connection.StopPlaying());
            AsyncButton("B · audition local sound in RAM",()=>Run(()=>audition.CandidateAsync(editing.Sounds[track].TargetId,editing.Sounds[track].Revision,editing.Sounds[track].State)),safe&&!soundAuditions.WorkspaceReplaced(track));
            AsyncButton("A · captured original",()=>Run(audition.OriginalAsync),safe);
            AsyncButton("Restore original and finish",()=>Run(audition.RestoreAsync),safe);
            AsyncButton("Keep B in device RAM only",()=>Run(()=>audition.KeepAsync(editing.Sounds[track].TargetId,editing.Sounds[track].Revision)),safe&&!soundAuditions.WorkspaceReplaced(track)&&audition.Status==SoundAuditionStatus.Candidate);
            AsyncButton("Read / reconcile hardware",()=>Run(audition.ReconcileAsync),ready);
            ActionButton("Abandon audition · preserve current device RAM",()=>soundAuditions.Abandon(track),!audition.Busy);
        });
    }
}

