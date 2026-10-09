using Sloop.Core.Sampling;
using Sloop.SampleEncoding;

namespace Sloop.Workstation;
public enum SampleMapping { Chops, DrumLanes, Instrument }
public sealed record KitSettings(string Name, SampleMapping Mapping, int RootNote, MonoChoice Mono, double Gain,bool Loop=false);
public static class SamplePreparation
{
    public static FitReport Measure(SampleDocument doc, IReadOnlyList<ChopAudio>? edits=null) => SlotBuilder.Measure(doc.Slices().Select(s=>
        checked((int)(((s.End-s.Start)*22050+ChopAudioSettings.Rate(doc.Source.SampleRate,ChopAudioSettings.Find(edits,s.Start,s.End))-1)/ChopAudioSettings.Rate(doc.Source.SampleRate,ChopAudioSettings.Find(edits,s.Start,s.End))))));
    public static ConvertedKit Build(SampleDocument doc, KitSettings settings, CancellationToken token=default, IReadOnlyList<ChopAudio>? edits=null)
    {
        if(!Measure(doc,edits).Fits) throw new ArgumentException("Trim the source to fit the slot before converting. No samples were dropped.");
        if(!Enum.IsDefined(settings.Mapping) || settings.RootNote is <0 or >127 || !double.IsFinite(settings.Gain) || settings.Gain is <0 or >8)
            throw new ArgumentException("Invalid kit settings (gain 0..8).");
        var slices=doc.Slices(); var zones=new List<SourceZone>();
        using var reader=new BinaryReader(File.OpenRead(doc.Source.Path));
        for(int index=0;index<slices.Length;index++) {
            token.ThrowIfCancellationRequested(); var s=slices[index]; var edit=ChopAudioSettings.Find(edits,s.Start,s.End); if(edit is not null) ChopAudioSettings.Validate(edit);
            var pcm=new double[checked((int)((s.End-s.Start)*doc.Source.Channels))];
            reader.BaseStream.Position=doc.Source.DataOffset+s.Start*doc.Source.Channels*2;
            for(int i=0;i<pcm.Length;i++) { if((i&4095)==0) token.ThrowIfCancellationRequested(); pcm[i]=reader.ReadInt16()/32768.0; }
            int root=settings.Mapping==SampleMapping.DrumLanes ? SlotBuilder.DrumLaneNote(index) : settings.RootNote+index;
            root=edit?.RootNote??root; if(root>127) throw new ArgumentException("Mapping exceeds MIDI note 127.");
            int? lo=settings.Mapping==SampleMapping.Instrument ? null : root;
            zones.Add(new(pcm,doc.Source.Channels,ChopAudioSettings.Rate(doc.Source.SampleRate,edit),new(settings.Mono,settings.Gain*(edit?.Gain??1)),root,lo,lo,
                settings.Loop?0:null,settings.Loop?checked((int)(s.End-s.Start)):null));
        }
        token.ThrowIfCancellationRequested();
        if(zones.Select(z=>z.RootNote).Distinct().Count()!=zones.Count) throw new ArgumentException("Each chop needs a distinct MIDI root; duplicate triggers would hide a zone.");
        return ConversionPipeline.Build(settings.Name,zones,token);
    }
    public static void WritePreview(string path, ReadOnlySpan<short> pcm)
    {
        using var w=new BinaryWriter(File.Create(path));
        w.Write("RIFF"u8); w.Write(36+pcm.Length*2); w.Write("WAVEfmt "u8); w.Write(16); w.Write((short)1);
        w.Write((short)1); w.Write(22050); w.Write(44100); w.Write((short)2); w.Write((short)16);
        w.Write("data"u8); w.Write(pcm.Length*2); foreach(short sample in pcm) w.Write(sample);
    }
}


