using Sloop.Core.Sampling;
using Sloop.SampleEncoding;
namespace Sloop.Workstation;

public static class QuickSampling
{
    public static SampleDocument View(SampleDocument source,bool playableSound) {
        if(!playableSound)return source;
        var one=new SampleDocument(source.Source);one.Trim(source.Start,source.End);return one;
    }
    public static int? EqualCount(SampleDocument doc) {
        int count=doc.Slices().Length;
        return doc.Markers.SequenceEqual(Enumerable.Range(1,count-1).Select(i=>doc.Start+(doc.End-doc.Start)*i/count))?count:null;
    }
    public static int FittingRate(SampleDocument document,IReadOnlyList<ChopAudio>? edits=null) {
        // eng_sample.c uses rate>>8: these rounded-Hz rates align with 128/64/48/32 native steps.
        foreach(int rate in new[]{22050,11025,8269,5513})if(SamplePreparation.Measure(document,edits,rate).Fits)return rate;
        throw new ArgumentException("Too long for one FM1 slot. Trim to about 29 seconds or less. Your original recording is kept.");
    }
    public static ConvertedKit Build(SampleDocument source,KitSettings options,bool playableSound,
        IReadOnlyList<ChopAudio>? edits=null,CancellationToken token=default) {
        var doc=View(source,playableSound);
        edits=edits?.Select(e=>e with{RootNote=null}).ToArray();
        options=options with{Mapping=playableSound?SampleMapping.Instrument:SampleMapping.Chops,Loop=false};
        return SamplePreparation.Build(doc,options,token,edits,FittingRate(doc,edits));
    }
}
