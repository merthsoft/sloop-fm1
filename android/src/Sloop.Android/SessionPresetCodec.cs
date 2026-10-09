using System.Globalization;
using System.Text.Json;
using Sloop.Workstation;

namespace Sloop.Android;

public static class SessionPresetCodec
{
    public static Dictionary<string,string> Encode(PerformOptions o){
        o.Mapping?.Validate();o.Macro?.Validate();
        return new(){
        ["Surface"]=o.Surface.ToString(),["Key"]=o.Key.ToString(),["Scale"]=o.Scale.ToString(),["Octave"]=o.Octave.ToString(),
        ["Velocity"]=o.Velocity.ToString(),["Tempo"]=o.Tempo.ToString(),["Division"]=o.Division.ToString(),["StrumMs"]=o.StrumMs.ToString(),
        ["Inversion"]=o.Inversion.ToString(),["VoiceLead"]=o.VoiceLead.ToString(),["Latch"]=o.Latch.ToString(),["BassTrack"]=o.BassTrack.ToString(),
        ["Style"]=o.Style.ToString(),["ChannelOverride"]=o.ChannelOverride.ToString(),["FollowHardwareOctave"]=o.FollowHardwareOctave.ToString(),
        ["perform.mapping.v1"]=JsonSerializer.Serialize(o.Mapping??new PerformanceMapping()),
        ["perform.macro.v1"]=JsonSerializer.Serialize(o.Macro??new PerformanceMacro())};
    }
    public static PerformOptions Decode(Dictionary<string,string> s){
        int I(string k,int min,int max){int n=int.Parse(s[k],CultureInfo.InvariantCulture);return n>=min&&n<=max?n:throw new InvalidDataException("Invalid preset "+k);}
        T E<T>(string k)where T:struct,Enum=>Enum.TryParse<T>(s[k],out var e)&&Enum.IsDefined(e)?e:throw new InvalidDataException("Invalid preset "+k);
        PerformanceMapping? mapping=null;PerformanceMacro? macro=null;
        try {
            if(s.TryGetValue("perform.mapping.v1",out var m)){if(m.Length>4096)throw new InvalidDataException("Mapping too large.");mapping=JsonSerializer.Deserialize<PerformanceMapping>(m)??throw new InvalidDataException("Missing mapping.");mapping.Validate();}
            if(s.TryGetValue("perform.macro.v1",out var x)){if(x.Length>4096)throw new InvalidDataException("Macro too large.");macro=JsonSerializer.Deserialize<PerformanceMacro>(x)??throw new InvalidDataException("Missing macro.");macro.Validate();}
        } catch(Exception error) when(error is JsonException or ArgumentException){throw new InvalidDataException("Invalid performance controls.",error);}
        return new(E<PerformSurface>("Surface"),I("Key",0,11),E<PerformanceScale>("Scale"),I("Octave",1,6),I("Velocity",1,127),I("Tempo",30,240),I("Division",1,8),I("StrumMs",5,200),I("Inversion",-3,3),bool.Parse(s["VoiceLead"]),bool.Parse(s["Latch"]),I("BassTrack",-1,3),E<PlayStyle>("Style"),I("ChannelOverride",-1,15),bool.Parse(s["FollowHardwareOctave"]),mapping,macro);
    }
}
