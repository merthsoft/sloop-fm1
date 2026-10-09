using Sloop.Workstation;
namespace Sloop.Android.Services;
public sealed partial class SampleWorkspace
{
    private bool chopSettingsInvalid;
    public ChopAudio[] ChopOptions {
        get {
            chopSettingsInvalid=false;
            if(Document is null) return [];
            var path=Document.Source.Path+".chopaudio";
            if(!File.Exists(path)) return [];
            try { if(new FileInfo(path).Length>16384) throw new ArgumentException("Chop settings too large."); var restored=ChopAudioSettings.Parse(File.ReadAllText(path)); if(restored.Any(e=>e.End>Document.Source.Frames)) throw new ArgumentException("Chop range exceeds source."); return restored; }
            catch(Exception e) { chopSettingsInvalid=true; Status="Could not restore chop audio: "+e.Message; return []; }
        }
    }
    public bool SetChopAudio(ChopAudio edit) {
        if(Busy || Document is null) return false;
        try {
            ChopAudioSettings.Validate(edit);
            if(!Document.Slices().Any(s=>s.Start==edit.Start&&s.End==edit.End)) throw new ArgumentException("Chop changed; select it again.");
            var active=Document.Slices();
            var edits=ChopOptions.Where(e=>active.Any(s=>s.Start==e.Start&&s.End==e.End)&& !(e.Start==edit.Start&&e.End==edit.End)).Append(edit).ToArray();
            WorkspaceFiles.StoreBytes(Document.Source.Path+".chopaudio",System.Text.Encoding.UTF8.GetBytes(ChopAudioSettings.Serialize(edits)));
            StopPreview();converted=null;Status="Chop audio saved; prepare kit again.";Notify();return true;
        } catch(Exception e) {Status="Could not save chop audio: "+e.Message;Notify();return false;}
    }
    public async Task<PitchEstimate?> EstimateChopAsync(int index) {
        if(Busy || Document is null) return null;
        var doc=Document;var slice=doc.Slices()[index];Busy=true;StopPreview();Notify();
        try {
            var estimate=await Task.Run(()=>SamplePitch.Estimate(doc.Source,slice.Start,slice.End));
            Status=estimate is null ? "No reliable pitch. Use manual root for noise, drums or short chops." : $"Estimate {SampleKitPlan.NoteName(estimate.RootNote)} · {estimate.Hertz:0.0} Hz · confidence {estimate.Confidence:P0}. Review before applying.";
            return estimate;
        } catch(Exception e) {Status="Pitch estimate failed: "+e.Message;return null;}
        finally {Busy=false;Notify();}
    }
    public async Task PreviewChopAsync(int index) {
        if(Busy || Document is null) return;
        var doc=Document;var slice=doc.Slices()[index];
        try {
            // Same conversion as kit preparation, including quantization and ADPCM decoding.
            var one=new Sloop.Core.Sampling.SampleDocument(doc.Source);one.SetSliceRange(0,slice.Start,slice.End);
            var options=KitOptions with {Mapping=SampleMapping.Chops};
            Busy=true;StopPreview();Notify();
            var edits=ChopOptions; if(chopSettingsInvalid) throw new ArgumentException("Repair or explicitly save chop settings before preview."); var epoch=previewEpoch;
            var result=await Task.Run(()=>SamplePreparation.Build(one,options,default,edits));
            if(epoch!=previewEpoch) {Status="Preview cancelled.";return;}
            var path=System.IO.Path.Combine(context.CacheDir!.AbsolutePath,"processed-chop.wav");
            SamplePreparation.WritePreview(path,result.Artifact.Preview[0].DecodedPcm);
            Busy=false;var wave=Sloop.Core.Sampling.PcmWave.Open(path);await PreviewAsync(0,wave.Frames,wave);
        } catch(Exception e) {Status="Chop preview failed: "+e.Message;}
        finally {Busy=false;Notify();}
    }
}



