using Android.Widget;
using System.Collections.Immutable;
using System.Text.Json;
using Sloop.Android.Services;
using Sloop.Scenes;
using Sloop.Sequencing;
using Sloop.Workstation;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    readonly object sceneGate=new();
    SceneCatalog sceneCatalog=new(1,[],[],[]);
    SceneScheduler sceneScheduler=new(Guid.NewGuid());
    string sceneStatus="App scenes restore local patches, pattern and tempo. FM1 sounds are applied separately.";
    string observedSceneState="";
    TransportTick? lastSceneTick;
    PreparedWorkspace? queuedSceneWorkspace;
    Guid? activeScene;
    bool applyingScene;
    ArrangementCursor? arrangementCursor;
    Dictionary<Guid,PreparedWorkspace>? arrangementWorkspaces;
    bool firstArrangementPhrase;
    long sceneRequestVersion;
    readonly Dictionary<Guid,int> automationValues=[];
    string ScenePath=>System.IO.Path.Combine(EffectiveWorkspaceRoot,"scenes","catalog.json");
    string SceneFingerprint()=>string.Join('|',editing.Sounds.Select(s=>s.Revision))+
        $"/{editing.Sequence.Current.Revision}/{editing.Native?.Current.Revision}/{editing.Tempo}/{samples.Document?.Source.Path}/{samples.Document?.Revision}/"+
        JsonSerializer.Serialize(SessionPresetCodec.Encode(performOptions))+string.Join(',',genericSequenceChannels)+string.Join(',',sequenceEditChannels);

    void InitializeScenes()
    {
        PrepareSessionExtrasAsync=root=>Task.Run<object?>(()=> {
            var path=System.IO.Path.Combine(root,"scenes","catalog.json");
            return File.Exists(path)?SceneStore.Load(path):new SceneCatalog(1,[],[],[]);
        });
        AdoptSessionExtras=value=> {
            sceneCatalog=(SceneCatalog)value!;
            if(!applyingScene){CancelQueuedScene();sceneScheduler=new(Guid.NewGuid());activeScene=null;}
            observedSceneState=SceneFingerprint();
        };
        CaptureSessionExtraAssets=()=>File.Exists(ScenePath)?[new SessionInput("scenes/catalog.json",ScenePath)]:[];
        CaptureSessionSceneIds=()=>sceneCatalog.Scenes.Select(s=>s.Id).ToArray();
        try{if(File.Exists(ScenePath))sceneCatalog=SceneStore.Load(ScenePath);}catch(Exception e){sceneStatus="Scene recovery required: "+e.Message;}
        observedSceneState=SceneFingerprint();
        connection.TransportTicked+=OnSceneTick;connection.TransportStopping+=CancelQueuedScene;
    }
    void DisposeScenes(){connection.TransportTicked-=OnSceneTick;connection.TransportStopping-=CancelQueuedScene;CancelQueuedScene();}
    void ObserveSceneWorkspace()
    {
        if(applyingScene)return;
        var fingerprint=SceneFingerprint();
        lock(sceneGate)if(observedSceneState!=fingerprint){CancelQueuedScene();sceneScheduler.ObserveLiveRevision(Guid.NewGuid());activeScene=null;observedSceneState=fingerprint;}
    }
    void CancelQueuedScene()
    {
        lock(sceneGate){
            sceneRequestVersion++;
            if(arrangementCursor is {} chain)sceneStatus=chain.Complete?"Arrangement complete.":"Arrangement stopped; current pattern can continue.";
            else if(sceneScheduler.Prepared is not null)sceneStatus="Queued scene cancelled.";
            if(sceneScheduler.Prepared is {} p)sceneScheduler.Cancel(p.Token);queuedSceneWorkspace=null;
            arrangementCursor=null;arrangementWorkspaces=null;automationValues.Clear();connection.CancelTransportBoundary();lastSceneTick=null;}
    }
    void SaveScenes(SceneCatalog catalog){
        if(connection.IsPlaying||connection.Snapshot.Busy)throw new IOException("Finish device work and stop sequence playback before editing the scene catalog.");
        SceneStore.Save(ScenePath,catalog);CancelQueuedScene();sceneCatalog=catalog;
        sceneScheduler.ObserveLiveRevision(Guid.NewGuid());activeScene=null;
    }
    void AddScenesEditor()
    {
        content.AddView(new Sloop.Scenes.Android.SceneEditor(this,sceneCatalog,sceneStatus,
            (id,boundary)=>_ = QueueSceneAsync(id,boundary),()=>{CancelQueuedScene();ShowWorkspace();},CaptureSceneDialog));
        content.AddView(Label("Scene switching changes app MIDI playback and local FM6 documents. It does not send an atomic hardware sound change.",12));
        ActionButton("Manage scenes",ManageScenesDialog,sceneCatalog.Scenes.Length>0&&!connection.IsPlaying);
        Section("scenes.hardware","Send scene sounds to FM1",AddSceneHardwareEditor);
        ActionButton("Create arrangement",()=>ArrangementDialog(null),!connection.IsPlaying);
        foreach(var chain in sceneCatalog.Arrangements){var chosen=chain;
            ButtonRow(($"Play · {chosen.Name}",()=>_ = PlayArrangementAsync(chosen),connection.CanPerform),
                ("Edit chain",()=>ArrangementDialog(chosen),!connection.IsPlaying));}
        ActionButton("Add MIDI CC sweep to a scene",AddSceneAutomationDialog,sceneCatalog.Scenes.Length>0&&!connection.IsPlaying);
    }
    void ManageScenesDialog()
    {
        new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Manage scenes")!
            .SetItems(sceneCatalog.Scenes.Select(s=>s.Name).ToArray(),(_,a)=> {
                var scene=sceneCatalog.Scenes[a.Which];
                new global::Android.App.AlertDialog.Builder(this)!.SetTitle(scene.Name)!
                    .SetItems(new[]{"Rename","Duplicate","Replace with current workspace","Delete"},(_,choice)=> {
                        if(choice.Which is 0 or 1){
                            var name=new EditText(this){Text=choice.Which==0?scene.Name:scene.Name+" copy",Hint="Scene name"};
                            new global::Android.App.AlertDialog.Builder(this)!.SetTitle(choice.Which==0?"Rename scene":"Duplicate scene")!.SetView(name)!
                                .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Save",(_,_)=>editing.Run(()=> {
                                    SaveScenes(choice.Which==0?CatalogEditing.Replace(sceneCatalog,scene.Id,scene.Revision,scene with{Name=name.Text??""}):
                                        CatalogEditing.Duplicate(sceneCatalog,scene.Id,name.Text??""));sceneStatus="Scene saved.";
                                },"Scene saved."))!.Show();
                        }else if(choice.Which==2){
                            new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Replace "+scene.Name+"?")!
                                .SetMessage("Capture the current three sounds, pattern and tempo into this scene. Arrangement references and compatible automation stay attached.")!
                                .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Replace",(_,_)=>editing.Run(()=> {
                                    if(samples.Busy||connection.IsRecording||connection.IsPlaying)throw new IOException("Finish sample work and stop playback/recording first.");
                                    var replacement=scene with{Tempo=editing.Tempo,Sounds=editing.Sounds.Select(s=>new SceneSound(s.TargetId,s.State)).ToImmutableArray(),AppPattern=(AppPattern)editing.Sequence.Current,NativePatterns=[]};
                                    SaveScenes(CatalogEditing.Replace(sceneCatalog,scene.Id,scene.Revision,replacement));sceneStatus="Replaced "+scene.Name;
                                },"Scene replaced."))!.Show();
                        }else{
                            new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Delete "+scene.Name+"?")!
                                .SetMessage("Deletes this scene and its automation. Scenes used by arrangements must be removed from those arrangements first.")!
                                .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Delete",(_,_)=>editing.Run(()=> {
                                    SaveScenes(CatalogEditing.Delete(sceneCatalog,scene.Id,scene.Revision));sceneStatus="Deleted "+scene.Name;
                                },"Scene deleted."))!.Show();
                        }
                    })!.Show();
            })!.Show();
    }
    void CaptureSceneDialog()
    {
        var name=new EditText(this){Hint="Scene name"};
        new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Capture current app scene")!.SetView(name)!
            .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Capture",(_,_)=>editing.Run(()=> {
                if(samples.Busy||connection.IsPlaying||connection.IsRecording)throw new IOException("Finish sample work and stop playback/recording first.");
                var scene=SceneEditing.Capture(name.Text??"",editing.Tempo,editing.Sounds.Select(s=>new SceneSound(s.TargetId,s.State)),(AppPattern)editing.Sequence.Current);
                SaveScenes(sceneCatalog with{Scenes=sceneCatalog.Scenes.Add(scene)});sceneStatus="Captured "+scene.Name;
            },"Scene saved locally."))!.Show();
    }
    async Task<PreparedWorkspace> PrepareSceneWorkspaceAsync(SceneSnapshot scene)
    {
        scene.Validate();
        if(scene.AppPattern is null||scene.Sounds.Length!=3||!scene.NativePatterns.IsEmpty)throw new NotSupportedException("This host restores complete app-pattern scenes with three local sounds.");
        if(sceneCatalog.Automation.Any(a=>a.SceneId==scene.Id&&(a.Lane.Target.Parameter!=AutomationParameter.MidiControlChange||!a.Lane.Target.TargetId.StartsWith("midi/channel:")||
            !int.TryParse(a.Lane.Target.TargetId.AsSpan("midi/channel:".Length),out int channel)||channel is <0 or >15)))
            throw new NotSupportedException("This host supports MIDI CC automation; hardware macro/native restores need a verified device adapter.");
        var generation=await Task.Run(()=>Generations.StageCopy(EffectiveWorkspaceRoot));
        await Task.Run(()=> {
            for(int track=0;track<3;track++){
                var target=$"local/track:{track}";var sound=scene.Sounds.Single(s=>s.TargetId==target);
                WorkspaceFiles.SaveSound(System.IO.Path.Combine(generation.Root,"workspaces",$"sound-{track}.sloop"),new(target,sound.State));
            }
            WorkspaceFiles.SavePattern(System.IO.Path.Combine(generation.Root,"workspaces","pattern.sloop"),scene.AppPattern);
            var path=System.IO.Path.Combine(generation.Root,"settings","performance.json");
            var values=JsonSerializer.Deserialize<Dictionary<string,string>>(File.ReadAllBytes(path))!;
            values["SequenceTempo"]=scene.Tempo.ToString();values["Tempo"]=scene.Tempo.ToString();
            WorkspaceFiles.StoreBytes(path,JsonSerializer.SerializeToUtf8Bytes(values));
        });
        return await PrepareWorkspaceGenerationAsync(generation);
    }
    async Task QueueSceneAsync(Guid id,SceneBoundary boundary)
    {
        long request=-1;
        try{
            if(samples.Busy||connection.Snapshot.Busy||connection.IsRecording)throw new IOException("Finish recording/sample/device work first.");
            samples.StopPreview();
            ObserveSceneWorkspace();SaveSessionSettings();CancelQueuedScene();request=sceneRequestVersion;var fingerprint=SceneFingerprint();
            var scene=sceneCatalog.Scenes.Single(s=>s.Id==id);var ready=await PrepareSceneWorkspaceAsync(scene);
            lock(sceneGate){
                if(request!=sceneRequestVersion)return;
                if(fingerprint!=SceneFingerprint())throw new IOException("Workspace changed while preparing the scene; try again.");
                arrangementCursor=null;arrangementWorkspaces=null;
                var prepared=sceneScheduler.Prepare(scene,sceneScheduler.LiveRevision,SceneRestoreMode.AppOnly);
                if(connection.IsPlaying){
                    var tick=lastSceneTick??throw new IOException("Wait for the transport to start.");
                    if(scene.AppPattern!.TicksPerQuarter!=tick.TicksPerQuarter)throw new IOException("Scene and running pattern need matching tick resolution.");
                    var position=Position(tick) with{Position=new((long)Math.Ceiling(connection.TransportPosition??tick.AbsoluteTick))};
                    var queued=sceneScheduler.Queue(prepared.Token,position,boundary);queuedSceneWorkspace=ready;connection.QueueTransportBoundary(queued.Due.Value);
                    sceneStatus=$"Queued {scene.Name} · {boundary} at tick {queued.Due.Value}";
                }else{
                    StopPerformance();var basis=new MusicalPosition(Guid.NewGuid(),new(0),scene.AppPattern!.TicksPerQuarter,4,scene.AppPattern.Length);
                    var due=sceneScheduler.Queue(prepared.Token,basis,SceneBoundary.Beat).Due;
                    var commit=sceneScheduler.Commit(basis with{Position=due})!;ApplySceneCommit(commit,ready,false);
                }
            }
            ShowWorkspace();
        }catch(Exception e){lock(sceneGate){if(request>=0&&request!=sceneRequestVersion)return;if(sceneScheduler.Prepared is {} p)sceneScheduler.Cancel(p.Token);queuedSceneWorkspace=null;connection.CancelTransportBoundary();}sceneStatus=e.Message;editing.SetStatus("Scene: "+e.Message);}
    }
    static MusicalPosition Position(TransportTick tick)=>new(tick.TransportEpoch,new(tick.AbsoluteTick),tick.TicksPerQuarter,4,new(tick.PatternLength),new(tick.PhraseOrigin));
    AppPattern OutputPattern(AppPattern pattern)=>connection.Snapshot.IsGenericMidi?pattern with{Notes=pattern.Notes.Select(n=>n with{Channel=n.Channel switch{0=>genericSequenceChannels[0],1=>genericSequenceChannels[1],2=>genericSequenceChannels[2],9=>genericSequenceChannels[3],_=>n.Channel}}).ToImmutableArray()}:pattern;
    void ApplySceneCommit(SceneCommit commit,PreparedWorkspace ready,bool running)
    {
        applyingScene=true;
        try{
            CommitWorkspaceGeneration(ready);
            if(running)connection.ReplaceLoopAtBoundary(OutputPattern(commit.Scene.AppPattern!),commit.Scene.Tempo);
            activeScene=commit.Scene.Id;automationValues.Clear();observedSceneState=SceneFingerprint();
            sceneScheduler.Reconcile(new(commit.Token,RestoreOutcome.Applied,"Local app scene adopted. Device sound state was not changed."));
            sceneStatus="Active · "+commit.Scene.Name+" · local sound snapshots";
        }catch(Exception e){sceneScheduler.Reconcile(new(commit.Token,RestoreOutcome.Unknown,e.Message));connection.StopPlaying();throw;}
        finally{applyingScene=false;}
    }
    void OnSceneTick(TransportTick tick)
    {
        bool transition;
        lock(sceneGate)transition=sceneScheduler.Pending?.Due.Value==tick.AbsoluteTick||(arrangementCursor is not null&&tick.IsLoop);
        if(!transition){ProcessSceneTick(tick);return;}
        // Publish editor state on its UI owner. The transport waits before dispatching new notes;
        // asset copying/validation already finished during preparation.
        var completed=new TaskCompletionSource(TaskCreationOptions.RunContinuationsAsynchronously);
        RunOnUiThread(()=>{try{ProcessSceneTick(tick);completed.TrySetResult();}catch(Exception e){completed.TrySetException(e);}});
        completed.Task.GetAwaiter().GetResult();
    }
    void ProcessSceneTick(TransportTick tick)
    {
        try{lock(sceneGate){
            lastSceneTick=tick;
            if((sceneScheduler.Pending?.Due.Value==tick.AbsoluteTick||(arrangementCursor is not null&&tick.IsLoop))&&
                (samples.Busy||connection.Snapshot.Busy||connection.IsRecording))throw new IOException("Scene switch cancelled: sample/device/recording work started after preparation.");
            if(sceneScheduler.Pending is {} pending&&tick.AbsoluteTick==pending.Due.Value){
                var commit=sceneScheduler.Commit(Position(tick))!;var ready=queuedSceneWorkspace??throw new IOException("Prepared scene unavailable.");queuedSceneWorkspace=null;
                ApplySceneCommit(commit,ready,true);RunOnUiThread(ShowWorkspace);return;
            }
            if(arrangementCursor is {} cursor&&tick.IsLoop){
                if(firstArrangementPhrase)firstArrangementPhrase=false;
                else {
                    var next=cursor.Advance();
                    if(next is null){connection.StopPlaying();return;}
                    if(next!=activeScene){
                        var scene=sceneCatalog.Scenes.Single(s=>s.Id==next);var ready=arrangementWorkspaces![next.Value];
                        // Prepared chains own complete snapshots; only known chain transitions may rebase.
                        ready=ready with{Generation=ready.Generation with{ExpectedPreviousRoot=EffectiveWorkspaceRoot}};
                        var prepared=sceneScheduler.Prepare(scene,sceneScheduler.LiveRevision,SceneRestoreMode.AppOnly);
                        var basis=Position(tick) with{Position=new(tick.AbsoluteTick-1),PhraseOrigin=new(tick.AbsoluteTick-tick.PatternLength)};
                        sceneScheduler.Queue(prepared.Token,basis,SceneBoundary.Phrase);
                        var commit=sceneScheduler.Commit(Position(tick))!;
                        ApplySceneCommit(commit,ready,true);firstArrangementPhrase=true;RunOnUiThread(ShowWorkspace);return;
                    }
                }
            }
            if(activeScene is {} id)foreach(var automation in sceneCatalog.Automation.Where(a=>a.SceneId==id)){
                var lane=automation.Lane;if(lane.Target.Parameter!=AutomationParameter.MidiControlChange)continue;
                if(!int.TryParse(lane.Target.TargetId.AsSpan("midi/channel:".Length),out int channel)||channel is <0 or >15)continue;
                var value=lane.Evaluate(new(tick.LoopTick),new(tick.PatternLength));
                if(value is {} v&&(!automationValues.TryGetValue(lane.Id,out int old)||old!=v)){connection.SendAutomationCc(channel,lane.Target.Index,v);automationValues[lane.Id]=v;}
            }
        }}catch(Exception e){connection.StopPlaying();RunOnUiThread(()=>{sceneStatus=e.Message;editing.SetStatus("Scene playback stopped: "+e.Message);});}
    }

    void ArrangementDialog(Arrangement? existing)
    {
        global::Android.App.AlertDialog? dialog=null;
        var steps=existing?.Steps.ToList()??[];var panel=new LinearLayout(this){Orientation=Orientation.Vertical};
        var name=new EditText(this){Hint="Arrangement name",Text=existing?.Name??""};panel.AddView(name);
        var loop=new CheckBox(this){Text="Loop arrangement",Checked=existing?.Loop??true};loop.SetTextColor(global::Android.Graphics.Color.White);panel.AddView(loop);
        var summary=Label("",14);panel.AddView(summary);
        var stepList=new LinearLayout(this){Orientation=Orientation.Vertical};
        var scroll=new ScrollView(this);scroll.AddView(stepList);panel.AddView(scroll,new LinearLayout.LayoutParams(-1,Dp(220)));
        void Refresh(){
            summary.Text=$"{steps.Count} steps · {steps.Sum(s=>s.Repeats)} phrases";stepList.RemoveAllViews();
            for(int index=0;index<steps.Count;index++){
                int selected=index;var step=steps[index];var label=new Button(this){Text=$"{index+1}. {sceneCatalog.Scenes.Single(x=>x.Id==step.SceneId).Name} × {step.Repeats}"};
                label.Click+=(_,_)=>new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Edit step "+(selected+1))!
                    .SetItems(new[]{"Set phrase repeats","Move earlier","Move later","Remove step"},(_,choice)=> {
                        if(choice.Which==0){
                            var repeats=new EditText(this){Text=steps[selected].Repeats.ToString(),InputType=global::Android.Text.InputTypes.ClassNumber};
                            new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Phrase repeats (1–1024)")!.SetView(repeats)!
                                .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Set",(_,_)=> {
                                    if(!int.TryParse(repeats.Text,out int count)||count is <1 or >1024){editing.SetStatus("Repeats must be 1–1024.");return;}
                                    steps[selected]=steps[selected] with{Repeats=count};Refresh();
                                })!.Show();
                        }else if(choice.Which==3){steps.RemoveAt(selected);Refresh();}
                        else{int target=selected+(choice.Which==1?-1:1);if(target>=0&&target<steps.Count){var moved=steps[selected];steps.RemoveAt(selected);steps.Insert(target,moved);Refresh();}}
                    })!.Show();stepList.AddView(label);
            }
            StyleContent(stepList);
        }
        Refresh();
        var add=new Button(this){Text="Add scene"};add.Click+=(_,_)=>new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Append scene")!
            .SetItems(sceneCatalog.Scenes.Select(s=>s.Name).ToArray(),(_,a)=>{steps.Add(new(sceneCatalog.Scenes[a.Which].Id,1));Refresh();})!.Show();panel.AddView(add);
        var remove=new Button(this){Text="Remove last step"};remove.Click+=(_,_)=>{if(steps.Count>0)steps.RemoveAt(steps.Count-1);Refresh();};panel.AddView(remove);StyleContent(panel);
        if(existing is not null){
            var delete=new Button(this){Text="Delete arrangement"};delete.Click+=(_,_)=>new global::Android.App.AlertDialog.Builder(this)!
                .SetTitle("Delete "+existing.Name+"?")!.SetMessage("The captured scenes will remain available.")!
                .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Delete",(_,_)=>editing.Run(()=> {
                    SaveScenes(sceneCatalog with{Arrangements=sceneCatalog.Arrangements.Where(a=>a.Id!=existing.Id).ToImmutableArray()});sceneStatus="Arrangement deleted.";dialog?.Dismiss();
                },"Arrangement deleted."))!.Show();panel.AddView(delete);StyleContent(delete);
        }
        dialog=new global::Android.App.AlertDialog.Builder(this)!.SetTitle(existing is null?"Create arrangement":"Edit arrangement")!.SetView(panel)!
            .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Save",(_,_)=>editing.Run(()=> {
                var chain=new Arrangement(existing?.Id??Guid.NewGuid(),name.Text??"",steps.ToImmutableArray(),loop.Checked);
                chain.Validate(sceneCatalog.Scenes.ToDictionary(s=>s.Id));SaveScenes(sceneCatalog with{Arrangements=sceneCatalog.Arrangements.Where(a=>a.Id!=chain.Id).Append(chain).ToImmutableArray()});
            },"Arrangement saved."))!.Show();
    }
    async Task PlayArrangementAsync(Arrangement chain)
    {
        long request=-1;
        try{
            if(!connection.CanPerform||samples.Busy)throw new IOException("Connect MIDI and finish sample work before playing an arrangement.");
            StopPerformance();await connection.StopPlayingAsync();SaveSessionSettings();CancelQueuedScene();request=sceneRequestVersion;var fingerprint=SceneFingerprint();
            var scenes=sceneCatalog.Scenes.ToDictionary(s=>s.Id);chain.Validate(scenes);
            var ready=new Dictionary<Guid,PreparedWorkspace>();
            foreach(var id in chain.Steps.Select(s=>s.SceneId).Distinct())ready[id]=await PrepareSceneWorkspaceAsync(scenes[id]);
            if(request!=sceneRequestVersion)return;
            if(fingerprint!=SceneFingerprint())throw new IOException("Workspace changed while preparing arrangement.");
            if(ready.Values.Select(p=>((AppPattern)p.Editing.Sequence.Current).TicksPerQuarter).Distinct().Count()!=1)throw new IOException("Arrangement patterns need matching tick resolution.");
            var first=scenes[chain.Steps[0].SceneId];var prepared=sceneScheduler.Prepare(first,sceneScheduler.LiveRevision,SceneRestoreMode.AppOnly);
            var basis=new MusicalPosition(Guid.NewGuid(),new(0),first.AppPattern!.TicksPerQuarter,4,first.AppPattern.Length);
            var due=sceneScheduler.Queue(prepared.Token,basis,SceneBoundary.Beat).Due;
            ApplySceneCommit(sceneScheduler.Commit(basis with{Position=due})!,ready[first.Id],false);
            arrangementWorkspaces=ready;arrangementCursor=new(chain,scenes);firstArrangementPhrase=true;
            await connection.LoopPatternAsync(OutputPattern(first.AppPattern),first.Tempo);
        }catch(Exception e){if(request>=0&&request!=sceneRequestVersion)return;CancelQueuedScene();editing.SetStatus("Arrangement: "+e.Message);}
    }
    void AddSceneAutomationDialog()
    {
        new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Scene for MIDI CC sweep")!.SetItems(sceneCatalog.Scenes.Select(s=>s.Name).ToArray(),(_,a)=> {
            var scene=sceneCatalog.Scenes[a.Which];var panel=new LinearLayout(this){Orientation=Orientation.Vertical};
            EditText Field(string hint,string text){var field=new EditText(this){Hint=hint,Text=text,InputType=global::Android.Text.InputTypes.ClassNumber};panel.AddView(Label(hint,14));panel.AddView(field);return field;}
            var channel=Field("MIDI channel (1–16)","1");var controller=Field("Controller (0–127)","74");var start=Field("Start value (0–127)","0");var end=Field("End value (0–127)","127");StyleContent(panel);
            new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Add linear CC sweep")!.SetView(panel)!.SetNegativeButton("Cancel",(_,_)=>{})!
                .SetPositiveButton("Save",(_,_)=>editing.Run(()=> {
                    int ch=int.Parse(channel.Text??"")-1;if(ch is <0 or >15)throw new ArgumentException("Channel must be 1–16.");
                    var lane=new AutomationLane(Guid.NewGuid(),new($"midi/channel:{ch}",AutomationParameter.MidiControlChange,int.Parse(controller.Text??""),0,127),AutomationShape.Linear,
                        [new(new(0),int.Parse(start.Text??"")),new(new(scene.AppPattern!.Length.Value-1),int.Parse(end.Text??""))]);
                    lane.Validate(scene.AppPattern.Length);SaveScenes(sceneCatalog with{Automation=sceneCatalog.Automation.Add(new(scene.Id,lane))});
                },"MIDI CC sweep saved. Values update at sixteenth-note transport ticks."))!.Show();
        })!.Show();
    }
}
