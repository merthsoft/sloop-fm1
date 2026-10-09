using Sloop.Android;
using Sloop.Workstation;
using Sloop.Core.Sampling;
int checks=0;
void Check(bool c,string m){checks++;if(!c)throw new Exception(m);}
void Reject(Action a,string m){try{a();}catch(InvalidDataException){checks++;return;}throw new Exception(m);}
var options=new PerformOptions(Mapping:new(){Degrees=[6,5,4,3,2,1,0]},Macro:new(true,2,new(1,10,90,20,MacroCurve.Squared),new(74,0,100,40)));
var encoded=SessionPresetCodec.Encode(options);
var decoded=SessionPresetCodec.Decode(encoded);
Check(decoded.Mapping!.Degrees.SequenceEqual(options.Mapping!.Degrees),"Mapped degrees survive saved preset.");
Check(decoded.Macro==options.Macro,"XY destination, ranges/defaults and curve survive preset.");
decoded.Mapping.Degrees[0]=0;
Check(SessionPresetCodec.Decode(encoded).Mapping!.Degrees[0]==6,"Decoded mappings have independent arrays.");
var legacy=new Dictionary<string,string>(encoded);legacy.Remove("perform.mapping.v1");legacy.Remove("perform.macro.v1");
Check(SessionPresetCodec.Decode(legacy).Mapping is null,"Older presets retain default mapping.");
Check(SessionPresetCodec.Decode(legacy).Macro is null,"Older presets keep macros disabled.");
foreach(var json in new[]{"null","{} broken","{\"Degrees\":[0]}","{\"Degrees\":null}","{\"Directions\":[999,0,0,0,0,0,0,0]}"}) {
 var invalid=new Dictionary<string,string>(encoded){["perform.mapping.v1"]=json};Reject(()=>SessionPresetCodec.Decode(invalid),"Malformed mapping rejected before adoption.");
}
foreach(var macro in new[]{new PerformanceMacro(true,3),new(true,0,new(64),new(74)),new(true,0,new(1),new(1)),new(true,0,new(1,50,40,45),new(74))}) {
 var invalid=new Dictionary<string,string>(encoded){["perform.macro.v1"]=System.Text.Json.JsonSerializer.Serialize(macro)};
 Reject(()=>SessionPresetCodec.Decode(invalid),"Unsafe macro rejected before adoption.");
}
var root=Path.Combine(Path.GetTempPath(),"sloop-settings-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);
try{
 string wavePath=Path.Combine(root,"source.wav");using(var stream=File.Create(wavePath))using(var writer=new BinaryWriter(stream)){
  writer.Write("RIFF"u8);writer.Write(36+2000);writer.Write("WAVEfmt "u8);writer.Write(16);writer.Write((short)1);writer.Write((short)1);writer.Write(22050);writer.Write(44100);writer.Write((short)2);writer.Write((short)16);writer.Write("data"u8);writer.Write(2000);writer.Write(new byte[2000]);
 }
 var source=PcmWave.Open(wavePath);ChopAudioSettings.ValidateFile(source);Check(true,"Legacy sample needs no chop sidecar.");
 File.WriteAllText(wavePath+".chopaudio",ChopAudioSettings.Serialize([new(0,1000,2,3,60)]));ChopAudioSettings.ValidateFile(source);Check(true,"Valid chop sidecar passes staged validation.");
 foreach(var json in new[]{"null","[null]","[{\"Start\":0,\"End\":1001}]","[{\"Start\":0,\"End\":100,\"Gain\":9}]",new string(' ',16385)}){
  File.WriteAllText(wavePath+".chopaudio",json);Reject(()=>ChopAudioSettings.ValidateFile(source),"Invalid chop sidecar rejects staged sample.");
 }
}finally{Directory.Delete(root,true);}
Console.WriteLine($"Session controls/sample adoption: {checks} checks passed.");
