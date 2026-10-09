using Sloop.Core.Sampling;
namespace Sloop.Workstation;

public sealed record ChopAudio(long Start, long End, double Gain = 1, double TuneSemitones = 0, int? RootNote = null);
public sealed record PitchEstimate(double Hertz, double MidiNote, double Confidence)
{
    public int RootNote => Math.Clamp((int)Math.Round(MidiNote), 0, 127);
}
public static class SamplePitch
{
    // Bounded 0.25-second analysis, strongest channel avoids opposite-phase cancellation.
    public static PitchEstimate? Estimate(PcmWave wave, long start, long end, CancellationToken token = default)
    {
        if(start < 0 || end > wave.Frames || end <= start) throw new ArgumentOutOfRangeException(nameof(start));
        int stride = Math.Max(1, wave.SampleRate / 8000), count = (int)Math.Min((end-start)/stride, 2048);
        if(count < 256) return null;
        double rate = wave.SampleRate/(double)stride;
        var channels = Enumerable.Range(0,wave.Channels).Select(_=>new double[count]).ToArray();
        using var reader = new BinaryReader(File.OpenRead(wave.Path));
        for(int i=0;i<count;i++) {
            token.ThrowIfCancellationRequested(); reader.BaseStream.Position=wave.DataOffset+(start+(long)i*stride)*wave.Channels*2;
            for(int c=0;c<wave.Channels;c++) channels[c][i]=reader.ReadInt16()/32768.0;
        }
        var x=channels.OrderByDescending(a=>a.Sum(v=>v*v)).First();
        double mean=x.Average(); for(int i=0;i<count;i++) x[i]-=mean;
        if(x.Sum(v=>v*v)/count < 0.00001) return null;
        int min=Math.Max(2,(int)(rate/1200)), max=Math.Min(count/2,(int)(rate/50));
        var corr=new double[max+1];
        for(int lag=min;lag<=max;lag++) {
            token.ThrowIfCancellationRequested(); double ab=0,aa=0,bb=0;
            for(int i=0;i<count-lag;i++){ ab+=x[i]*x[i+lag];aa+=x[i]*x[i];bb+=x[i+lag]*x[i+lag]; }
            corr[lag]=ab/Math.Sqrt(Math.Max(1e-30,aa*bb));
        }
        int best=-1;
        for(int lag=min+1;lag<max;lag++) if(corr[lag]>.75 && corr[lag]>=corr[lag-1] && corr[lag]>corr[lag+1]) {best=lag;break;}
        if(best<0) return null;
        double divisor=corr[best-1]-2*corr[best]+corr[best+1];
        double offset=Math.Abs(divisor)>1e-12 ? .5*(corr[best-1]-corr[best+1])/divisor : 0;
        double hz=rate/(best+Math.Clamp(offset,-.5,.5));
        return new(hz,69+12*Math.Log2(hz/440),Math.Clamp(corr[best],0,1));
    }
}
public static class ChopAudioSettings
{
    public static void ValidateFile(PcmWave source)
    {
        string path=source.Path+".chopaudio";
        if(!File.Exists(path))return;
        if(new FileInfo(path).Length>16384)throw new InvalidDataException("Chop settings too large.");
        try {if(Parse(File.ReadAllText(path)).Any(e=>e.End>source.Frames))throw new InvalidDataException("Chop setting extends beyond the source.");}
        catch(Exception error) when(error is ArgumentException or System.Text.Json.JsonException){throw new InvalidDataException("Invalid chop settings.",error);}
    }
    public static void Validate(ChopAudio edit)
    {
        ArgumentNullException.ThrowIfNull(edit);
        if(edit.Start<0 || edit.End<=edit.Start || !double.IsFinite(edit.Gain) || edit.Gain is <0 or >8 || !double.IsFinite(edit.TuneSemitones) || edit.TuneSemitones is <-24 or >24 || edit.RootNote is <0 or >127)
            throw new ArgumentException("Use gain 0–8, tuning −24…24 and root 0–127.");
    }
    public static int Rate(int rate, ChopAudio? edit) { if(edit is not null) Validate(edit); return checked((int)Math.Round(rate*Math.Pow(2,(edit?.TuneSemitones??0)/12))); }
    public static ChopAudio? Find(IReadOnlyList<ChopAudio>? edits,long start,long end) => edits?.FirstOrDefault(e=>e.Start==start&&e.End==end);
    public static string Serialize(IReadOnlyList<ChopAudio> edits) { foreach(var e in edits) Validate(e); return System.Text.Json.JsonSerializer.Serialize(edits); }
    public static ChopAudio[] Parse(string json) {
        if(json.Length>16384) throw new ArgumentException("Chop settings too large.");
        var edits=System.Text.Json.JsonSerializer.Deserialize<ChopAudio[]>(json)??throw new ArgumentException("Missing chop settings.");
        if(edits.Length>16 || edits.Any(e=>e is null) || edits.Select(e=>(e.Start,e.End)).Distinct().Count()!=edits.Length) throw new ArgumentException("Invalid chop settings count, null entry or duplicate ranges.");
        foreach(var e in edits) Validate(e); return edits;
    }
}

