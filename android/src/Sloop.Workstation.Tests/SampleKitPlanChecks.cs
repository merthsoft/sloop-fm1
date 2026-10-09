using Sloop.Core.Sampling;
using Sloop.SampleEncoding;
using Sloop.Workstation;

public static class SampleKitPlanChecks
{
    public static int Run()
    {
        int checks = 0;
        void Check(bool condition, string message) { if (!condition) throw new Exception(message); checks++; }
        void Reject(KitSettings options, int count) {
            try { SampleKitPlan.Assignments(options,count); } catch (ArgumentException) { checks++; return; }
            throw new Exception("Invalid mapping accepted.");
        }
        var path = Path.Combine(Path.GetTempPath(),"sloop-kit-plan-"+Guid.NewGuid().ToString("N")+".wav");
        try {
            SamplePreparation.WritePreview(path,Enumerable.Range(0,160).Select(i=>(short)(i*100)).ToArray());
            var original = File.ReadAllBytes(path);
            var doc = new SampleDocument(PcmWave.Open(path));
            foreach (int count in new[] { 1, 3, 16 }) {
                doc.EqualParts(count);
                foreach (var mapping in Enum.GetValues<SampleMapping>()) {
                    var options = new KitSettings("PLAN",mapping,60,MonoChoice.FirstChannel,1);
                    var planned = SampleKitPlan.Assignments(options,count);
                    var artifact = SamplePreparation.Build(doc,options).Artifact;
                    foreach (var preview in artifact.Preview) {
                        var assignment = planned[preview.InputIndex];
                        Check(assignment.ChopIndex == preview.InputIndex && assignment.RootNote == preview.RootNote
                            && assignment.LowNote == preview.LowNote && assignment.HighNote == preview.HighNote,
                            "Source-order assignment disagrees with encoded header.");
                    }
                    Check(artifact.Fit.DataBytes==SamplePreparation.Measure(doc).DataBytes,"Chop plan capacity disagrees with prepared kit.");
                }
            }
            var drum = SampleKitPlan.Assignments(new("D",SampleMapping.DrumLanes,127,MonoChoice.FirstChannel,1),16);
            Check(drum[0].RootNote==36 && drum[1].RootNote==35,"Drum chops must retain lane order, not sorted wire order.");
            Check(SampleKitPlan.NoteName(60)=="C4" && SampleKitPlan.NoteName(0)=="C-1" && SampleKitPlan.NoteName(127)=="G9","MIDI octave labels.");
            Reject(new("X",SampleMapping.Chops,127,MonoChoice.FirstChannel,1),2);
            Reject(new("X",SampleMapping.Instrument,113,MonoChoice.FirstChannel,1),16);
            Reject(new("X",(SampleMapping)99,60,MonoChoice.FirstChannel,1),1);
            Reject(new("X",SampleMapping.Chops,60,(MonoChoice)99,1),1);
            Reject(new("X",SampleMapping.Chops,60,MonoChoice.FirstChannel,double.NaN),1);
            Reject(new("X",SampleMapping.Chops,60,MonoChoice.FirstChannel,1),0);
            Check(SampleKitPlan.Assignments(new("X",SampleMapping.Chops,112,MonoChoice.FirstChannel,1),16)[15].RootNote==127,"Highest legal first root.");
            Check(File.ReadAllBytes(path).SequenceEqual(original),"Planning and conversion must preserve the source WAV.");
            return checks;
        } finally { File.Delete(path); }
    }
}
