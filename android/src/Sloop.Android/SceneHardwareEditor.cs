using Android.Widget;
using Sloop.Android.Services;

namespace Sloop.Android;

public sealed partial class MainActivity
{
    SceneHardwareBaseline? sceneHardwareBaseline;
    bool sceneHardwareUnknown;
    string sceneHardwareStatus="Read the three FM6 destinations before sending scene sounds.";

    void AddSceneHardwareEditor()
    {
        content.AddView(Label(sceneHardwareStatus,14));
        content.AddView(Label("Scene Synth 1 → FM1 Synth 1; Synth 2 → Synth 2; Synth 3 → Synth 3. All three destinations must already use FM6. Stop the FM1 song and release its keys before Read/Send. Keep its controls unchanged during transfer.",12));
        bool available=connection.Snapshot.Device?.ProtocolVersion>=9&&!connection.Snapshot.IsGenericMidi&&
            !connection.Snapshot.IsSimulated&&!connection.Snapshot.Busy&&!connection.IsPlaying&&!connection.IsRecording&&!samples.Busy;
        AsyncButton("Read all three FM1 sound destinations",async()=> {
            StopPerformance();await connection.StopPlayingAsync();
            sceneHardwareBaseline=null;
            var baseline=await connection.ReadSceneSoundsAsync([0,1,2]);
            sceneHardwareBaseline=baseline;sceneHardwareUnknown=false;
            sceneHardwareStatus="Verified hardware baseline · "+string.Join(" · ",baseline.Tracks.Select(t=>$"Synth {t.Track+1}: {t.State.Patch.Name}"));
            ShowWorkspace();
        },available);
        ActionButton("Choose scene sounds to send",ChooseHardwareScene,available&&sceneHardwareBaseline is not null&&!sceneHardwareUnknown&&sceneCatalog.Scenes.Length>0);
        content.AddView(Label("This transfers sound patches and macros to RAM with readback. It does not send the pattern, samples or tempo. Sequential transfer takes time; live beat-boundary sound switching is not available yet.",12));
    }

    void ChooseHardwareScene()
    {
        new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Send sounds from scene")!
            .SetItems(sceneCatalog.Scenes.Select(s=>s.Name).ToArray(),(_,choice)=> {
                var scene=sceneCatalog.Scenes[choice.Which];
                new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Send "+scene.Name+" sounds?")!
                    .SetMessage("Replaces the three FM1 FM6 sounds in RAM. Synth 1 → 1, Synth 2 → 2, Synth 3 → 3. Stop the physical FM1 song and leave its panel untouched. Each sound is checked against the baseline and read back; changes are sequential.")!
                    .SetNegativeButton("Cancel",(_,_)=>{})!.SetPositiveButton("Send & verify",async(_,_)=> {
                        try{
                            if(sceneHardwareUnknown)throw new IOException("Read hardware again to resolve the previous unknown result.");
                            if(samples.Busy||connection.Snapshot.Busy||connection.IsPlaying||connection.IsRecording)throw new IOException("Finish sample/device work and stop playback/recording first.");
                            var baseline=sceneHardwareBaseline??throw new IOException("Read the FM1 destinations first.");
                            if(!scene.NativePatterns.IsEmpty||scene.Sounds.Length!=3)throw new IOException("Choose a scene with three local FM6 sounds.");
                            var targets=Enumerable.Range(0,3).Select(track=>new SceneHardwareTarget(track,scene.Sounds.Single(s=>s.TargetId==$"local/track:{track}").State)).ToArray();
                            StopPerformance();await connection.StopPlayingAsync();
                            sceneHardwareBaseline=null;
                            var result=await connection.ApplySceneSoundsAsync(baseline,targets);
                            Array.Clear(hardwareSound);
                            sceneHardwareUnknown=result.Tracks.Any(t=>t.Outcome==SceneHardwareOutcome.Unknown);
                            sceneHardwareStatus=(result.Applied?"Scene sounds verified: "+scene.Name:"Scene sound transfer requires review.")+"\n"+
                                string.Join("\n",result.Tracks.Select(t=>$"Synth {t.Track+1}: {t.Outcome}"+
                                    (t.Observed is {} observed?$" · observed {observed.State.Patch.Name}, algorithm {observed.State.Patch.Algorithm}":" · no complete readback")+
                                    (t.Error is {} error?" · "+error:"")));
                            editing.SetStatus(sceneHardwareStatus+"\nRead the destinations again before another transfer.");ShowWorkspace();
                        }catch(Exception e){sceneHardwareBaseline=null;sceneHardwareStatus=e.Message;editing.SetStatus("Scene sounds: "+e.Message);ShowWorkspace();}
                    })!.Show();
            })!.Show();
    }
}
