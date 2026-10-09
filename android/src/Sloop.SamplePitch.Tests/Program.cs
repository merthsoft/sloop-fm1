using Sloop.Core.Sampling;
using Sloop.Workstation;
using Sloop.SampleEncoding;
using System.Buffers.Binary;
int checks=0;
void Check(bool ok,string name){if(!ok)throw new Exception(name);checks++;}
var path=Path.Combine(Path.GetTempPath(),Guid.NewGuid()+".wav");
try {
    foreach(double hz in new[]{110.0,220,440,880}) {
        SamplePreparation.WritePreview(path,Enumerable.Range(0,11025).Select(i=>(short)(12000*Math.Sin(2*Math.PI*hz*i/22050))).ToArray());
        Check(SamplePitch.Estimate(PcmWave.Open(path),0,100) is null,"short chop rejected");
        var pitch=SamplePitch.Estimate(PcmWave.Open(path),0,11025);
        Check(pitch is not null&&Math.Abs(pitch.Hertz-hz)<hz*.01&&pitch.Confidence>.95,"sine pitch "+hz);
    }
    var wave=PcmWave.Open(path);var bytes=File.ReadAllBytes(path);var doc=new SampleDocument(wave);
    var settings=new KitSettings("TEST",SampleMapping.Chops,60,MonoChoice.AverageChannels,1);
    var edits=new[]{new ChopAudio(0,11025,.5,12,69)};
    var kit=SamplePreparation.Build(doc,settings,default,edits);
    Check(kit.Artifact.Preview[0].RootNote==69,"preview root");
    Check(BinaryPrimitives.ReadInt16LittleEndian(kit.Artifact.Header.AsSpan(52))==69*16,"actual header root");
    Check(BinaryPrimitives.ReadUInt32LittleEndian(kit.Artifact.Header.AsSpan(48))==32768,"fixed encoded rate");
    Check(kit.Artifact.Fit.Samples==5513,"octave duration");
    Check(SamplePreparation.Measure(doc,edits).DataBytes==kit.Artifact.Fit.DataBytes,"tuned fit");
    Check(kit.Reports[0].PeakBeforeQuantization < .19,"gain applied");
    Check(bytes.SequenceEqual(File.ReadAllBytes(path)),"immutable original");
    Check(ChopAudioSettings.Parse(ChopAudioSettings.Serialize(edits)).SequenceEqual(edits),"persistence");
    doc.EqualParts(2);
    Check(ChopAudioSettings.Find(edits,doc.Slices()[0].Start,doc.Slices()[0].End) is null,"changed boundary does not inherit edits");
    var roots=new[]{new ChopAudio(0,doc.Slices()[0].End,1,0,72),new ChopAudio(doc.Slices()[1].Start,11025,1,0,48)};
    var inst=settings with {Mapping=SampleMapping.Instrument};var assignments=SampleKitPlan.Assignments(inst,2,doc,roots);var built=SamplePreparation.Build(doc,inst,default,roots);
    Check(built.Artifact.Preview.All(p=>assignments[p.InputIndex].RootNote==p.RootNote&&assignments[p.InputIndex].LowNote==p.LowNote&&assignments[p.InputIndex].HighNote==p.HighNote),"sorted ranges match headers");
    SamplePreparation.WritePreview(path,new short[11025]);Check(SamplePitch.Estimate(PcmWave.Open(path),0,11025) is null,"silence rejected");
    try{ChopAudioSettings.Parse("[{\"Start\":0,\"End\":10,\"Gain\":9}]");throw new Exception("invalid accepted");}catch(ArgumentException){checks++;}
    using var cancel=new CancellationTokenSource();cancel.Cancel();try{SamplePitch.Estimate(wave,0,11025,cancel.Token);throw new Exception("cancel ignored");}catch(OperationCanceledException){checks++;}
        var random=new Random(42);SamplePreparation.WritePreview(path,Enumerable.Range(0,11025).Select(_=>(short)random.Next(-12000,12000)).ToArray());
    Check(SamplePitch.Estimate(PcmWave.Open(path),0,11025) is null,"noise rejected");
    SamplePreparation.WritePreview(path,Enumerable.Range(0,11025).Select(i=>(short)(10000*Math.Sin(2*Math.PI*220*i/22050))).ToArray());
    var looping=SamplePreparation.Build(new SampleDocument(PcmWave.Open(path)),settings with {Loop=true},default,new[]{new ChopAudio(0,11025,1,-12)});
    Check(looping.Artifact.Fit.Samples==22050,"down tuning duration");
    Check(BinaryPrimitives.ReadUInt32LittleEndian(looping.Artifact.Header.AsSpan(44))==22049&&looping.Artifact.Header[59]==1,"tuned full loop boundaries");
    SamplePreparation.WritePreview(path,looping.Artifact.Preview[0].DecodedPcm);
    Check(Math.Abs(SamplePitch.Estimate(PcmWave.Open(path),0,22050)!.Hertz-110)<1,"encoded down tuning pitch");
        using(var writer=new BinaryWriter(File.Create(path))) {
        writer.Write("RIFF"u8);writer.Write(36+11025*4);writer.Write("WAVEfmt "u8);writer.Write(16);writer.Write((short)1);writer.Write((short)2);writer.Write(22050);writer.Write(88200);writer.Write((short)4);writer.Write((short)16);writer.Write("data"u8);writer.Write(11025*4);
        for(int i=0;i<11025;i++) {short v=(short)(10000*Math.Sin(2*Math.PI*440*i/22050));writer.Write(v);writer.Write((short)-v);}
    }
    Check(Math.Abs(SamplePitch.Estimate(PcmWave.Open(path),0,11025)!.Hertz-440)<2,"opposite stereo pitch");
    var duplicate=new[]{new ChopAudio(0,doc.Slices()[0].End,1,0,60),new ChopAudio(doc.Slices()[1].Start,11025,1,0,60)};
    try {SampleKitPlan.Assignments(inst,2,doc,duplicate);throw new Exception("duplicates accepted");}catch(ArgumentException){checks++;}
    Console.WriteLine($"{checks} sampling pitch/audio checks passed.");
} finally {File.Delete(path);}



Console.WriteLine($"Existing kit mapping regression: {SampleKitPlanChecks.Run()} checks passed.");

