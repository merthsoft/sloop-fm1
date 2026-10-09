using System.Collections.Immutable;
using System.Security.Cryptography;
using System.Text.Json;
using System.Text.RegularExpressions;
using Sloop.SoundDesign;

try
{
int assertions=0;
void Check(bool value,string message)
{ assertions++;if(!value)throw new Exception(message); }
void Reject(Action action,string message)
{
    assertions++;
    try {action();}catch(ArgumentException){return;}catch(InvalidOperationException){return;}
    throw new Exception("Expected rejection: "+message);
}
void Equal(Patch a,Patch b,string message)=>Check(PatchCodec.ContentEquals(a,b),message);
string golden=Path.Combine(AppContext.BaseDirectory,"Golden");
string? root=AppContext.BaseDirectory;
while(root!=null&&!File.Exists(Path.Combine(root,"firmware/src/eng_fm6.c")))root=Directory.GetParent(root)?.FullName;
Check(root!=null,"Run tests from the repository checkout to verify source provenance.");
using var manifest=JsonDocument.Parse(File.ReadAllText(Path.Combine(golden,"manifest.json")));
Check(Convert.ToHexString(SHA256.HashData(File.ReadAllBytes(Path.Combine(root!,"tools/gen_fm6_patches.py")))).ToLowerInvariant()
    ==manifest.RootElement.GetProperty("sha256").GetString(),"Factory generator changed: review and regenerate fixtures.");
string web=File.ReadAllText(Path.Combine(root!,"web/editor.html"));
var webBlock=web.Split("const FACTORY_PK = [")[1].Split("];",2)[0];
var webVectors=Regex.Matches(webBlock,@"\[([\d,\s]+)\]").Select(m=>m.Groups[1].Value.Split(',').Select(x=>byte.Parse(x.Trim())).ToArray()).ToArray();
Check(webVectors.Length==8,"Eight web factory voices.");
for(int i=0;i<8;i++)
{
    var v=File.ReadAllBytes(Path.Combine(golden,$"{i}.voice.bin"));
    var packed=File.ReadAllBytes(Path.Combine(golden,$"{i}.packed.bin"));
    var p=PatchCodec.DecodeVoice(v);
    Check(PatchCodec.Pack(p).SequenceEqual(packed),$"Golden pack {i}");
    Check(PatchCodec.EncodeVoice(PatchCodec.Unpack(packed)).SequenceEqual(v),$"Golden unpack {i}");
    Check(packed.SequenceEqual(webVectors[i]),$"Web factory parity {i}");
    Equal(p,FactoryLibrary.Get(i),$"Embedded factory parity {i}");
    Check(p.Name==manifest.RootElement.GetProperty("voices")[i].GetProperty("name").GetString(),"Name conversion");
    Check(PatchCodec.EncodeVoice(p)[16]==p.GetOperator(6).OutputLevel,"OP6 first");
    Check(PatchCodec.EncodeVoice(p)[134]==p.Algorithm-1,"Zero-based wire algorithm");
}
// Verify the role map against all 32 current firmware graphs, not operator-number guesses.
string core=File.ReadAllText(Path.Combine(root!,"firmware/src/fm6_core.c"));
var graphs=Regex.Matches(core.Split("static const uint8_t FM6_ALG[32][6]")[1].Split("};",2)[0],@"\{([^{}]+)\}");
Check(graphs.Count==32,"All firmware topologies.");
for(int a=0;a<32;a++)
{
    var flags=graphs[a].Groups[1].Value.Split(',').Select(x=>Convert.ToInt32(x.Trim(),16)).ToArray();
    for(int k=0;k<6;k++)Check(AlgorithmTopology.IsCarrier(a+1,6-k)==((flags[k]&3)==0),$"Topology {a+1}/{k}");
}
int[] opMax=[99,99,99,99,99,99,99,99,99,99,99,3,3,7,3,7,99,1,31,99,14];
int[] voiceMax=[99,99,99,99,99,99,99,99,31,7,1,99,99,99,99,1,5,7,48];
string engine=File.ReadAllText(Path.Combine(root!,"firmware/src/eng_fm6.c"));
foreach(var (label,expected) in new[]{("OPMAX",opMax),("VMAX",voiceMax)})
{
    var match=Regex.Match(engine,label+@"\[\d+\] = \{([^}]+)\}");
    Check(match.Groups[1].Value.Split(',').Select(x=>int.Parse(x.Trim())).SequenceEqual(expected),label+" matches firmware bounds");
}
var basis=FactoryLibrary.Get(0);
for(int field=0;field<145;field++)
{
    var v=PatchCodec.EncodeVoice(basis);int max=field<126?opMax[field%21]:voiceMax[field-126];
    v[field]=0;Equal(PatchCodec.DecodeVoice(v),PatchCodec.Unpack(PatchCodec.Pack(PatchCodec.DecodeVoice(v))),$"Min {field}");
    v[field]=(byte)max;Check(PatchCodec.EncodeVoice(PatchCodec.DecodeVoice(v))[field]==max,$"Max {field}");
    v[field]=(byte)(max+1);Reject(()=>PatchCodec.DecodeVoice(v),$"Overflow {field}");
}
for(int field=145;field<155;field++)
{
    var v=PatchCodec.EncodeVoice(basis);v[field]=31;Reject(()=>PatchCodec.DecodeVoice(v),"Control name byte");
    v[field]=127;Reject(()=>PatchCodec.DecodeVoice(v),"DEL name byte");
}
Reject(()=>PatchCodec.DecodeVoice(new byte[154]),"Voice length");
Reject(()=>PatchCodec.Unpack(new byte[127]),"Packed length");
var bad=PatchCodec.Pack(basis);bad[0]=128;Reject(()=>PatchCodec.Unpack(bad),"Not 7-bit");
bad=PatchCodec.Pack(basis);bad[12]=127;Reject(()=>PatchCodec.Unpack(bad),"Invalid packed detune");
bad=PatchCodec.Pack(basis);bad[116]=12;Reject(()=>PatchCodec.Unpack(bad),"Invalid packed LFO wave");
bad=PatchCodec.Pack(basis);bad[117]=49;Reject(()=>PatchCodec.Unpack(bad),"Invalid packed transpose");
Reject(()=>PatchCodec.Pack(basis with{Algorithm=0}),"Negative wire algorithm");
Reject(()=>PatchCodec.Pack(basis with{Name="é"}),"Non-ASCII name");
Reject(()=>PatchCodec.Pack(basis with{Name="12345678901"}),"Long name");
Reject(()=>PatchCodec.Pack(basis with{Operators=ImmutableArray<Operator>.Empty}),"Operator count");
Check(PatchValidation.DeviceName("écho long title")==" cho long ","Explicit device-name conversion");
var fixedPatch=basis.WithOperator(3,basis.GetOperator(3) with{Mode=FrequencyMode.Fixed,Coarse=31,Fine=99,Detune=14});
Equal(fixedPatch,PatchCodec.Unpack(PatchCodec.Pack(fixedPatch)),"Fixed mode preserved");
var single=SysExCodec.ExportSingle(basis,15);
Check(single.Length==163&&single[4]==1&&single[5]==27,"Single SysEx layout");
Equal(basis,SysExCodec.Import(single).Voices.Single(),"Single import");
Check(SysExCodec.Import(single).Channel==15,"Channel preserved");
var bank=SysExCodec.ExportBank(Enumerable.Range(0,32).Select(i=>FactoryLibrary.Get(i%8)));
Check(bank.Length==4104&&bank[4]==32&&bank[5]==0,"Bank layout");
var asset=SysExCodec.Import(bank);
Check(asset.Voices.Length==32,"Full bank import");
for(int i=0;i<32;i++)Equal(asset.Voices[i],FactoryLibrary.Get(i%8),"Bank voice equality");
Check(asset.ExportOriginal().SequenceEqual(bank),"Original asset lossless");
// Firmware ignores unused packed bits; canonical export strips them, original export retains them.
bank[6+11]|=64;bank[^2]=SysExCodec.Checksum(bank.AsSpan(6,4096));
asset=SysExCodec.Import(bank);
Check(asset.ExportOriginal().SequenceEqual(bank),"Unused bits retained in original asset");
Check(!SysExCodec.ExportBank(asset.Voices).SequenceEqual(bank),"Canonical bank deliberately separate");
single[^2]^=1;Reject(()=>SysExCodec.Import(single),"Bad checksum");
Reject(()=>SysExCodec.Import(bank[..^1]),"Short bank");
single=SysExCodec.ExportSingle(basis);single[5]--;Reject(()=>SysExCodec.Import(single),"Bad declared size");
single=SysExCodec.ExportSingle(basis);single[3]=4;Reject(()=>SysExCodec.Import(single),"Unsupported 4-op format");
Reject(()=>SysExCodec.ExportBank([basis]),"No silent bank padding");
Reject(()=>SysExCodec.ExportSingle(basis,16),"Channel bounds");
var rng=new Random(643);
for(int sample=0;sample<300;sample++)
{
    var v=PatchCodec.EncodeVoice(basis);
    for(int f=0;f<145;f++)v[f]=(byte)rng.Next((f<126?opMax[f%21]:voiceMax[f-126])+1);
    for(int f=145;f<155;f++)v[f]=(byte)rng.Next(32,127);
    Check(PatchCodec.EncodeVoice(PatchCodec.Unpack(PatchCodec.Pack(PatchCodec.DecodeVoice(v)))).SequenceEqual(v),"Random legal roundtrip");
}
var state=new SoundState(basis,new(0,2,8,1,-10,2,12));
var intent=new SoundIntent(SoundFamily.Keys,42);
var locked=new PatchLocks(LockGroup.Algorithm|LockGroup.Tuning|LockGroup.Attack,ImmutableHashSet.Create(2));
foreach(var family in Enum.GetValues<SoundFamily>())
{
    var fi=intent with{Family=family};
    var drafts=ProceduralDesigner.Generate("track:0",0,state,fi,locked);
    var again=ProceduralDesigner.Generate("track:0",0,state,fi,locked);
    Check(drafts.Length==4,"Four drafts");
    for(int d=0;d<4;d++)
    {
        Equal(drafts[d].After.Patch,again[d].After.Patch,"Seed reproducibility");
        Equal(drafts[d].After.Patch,locked.Enforce(basis,drafts[d].After.Patch),"Generation respects locks");
        Check(drafts[d].After.Macros==MacroContext.Neutral,"Declared baseline");
        Check(drafts[d].Audition.Notes.Select(n=>n.Velocity).Distinct().SequenceEqual(new[]{40,80,120}),"Velocity plan");
        Check(drafts[d].Audition.Notes.All(n=>n.MidiNote is >=0 and <=127 && n.LengthSteps>0),"Playable phrase metadata");
    }
}
var allLocks=new PatchLocks(LockGroup.VoiceControls|LockGroup.Tuning|LockGroup.Envelopes,Enumerable.Range(1,6).ToImmutableHashSet());
var edits=Enum.GetValues<RefinementDimension>().Select(d=>new Refinement(d,50)).ToArray();
var noOp=ProceduralDesigner.Refine("track:0",0,state,intent,edits,allLocks);
Check(noOp.IsNoOp,"Full locks are a true no-op");
foreach(var group in new[]{LockGroup.Envelopes,LockGroup.Attack,LockGroup.Tuning,LockGroup.VoiceControls})
{
    var partial=new PatchLocks(group,ImmutableHashSet<int>.Empty);
    foreach(int sign in new[]{-1,1})
    {
        var result=ProceduralDesigner.Refine("track:0",0,state,intent,edits.Select(e=>e with{Amount=e.Amount*sign}),partial);
        Equal(result.After.Patch,partial.Enforce(basis,result.After.Patch),$"Partial {group} locks survive {sign}");
        Check(result.After.Macros==state.Macros,"Refinement macro preservation");
    }
}
var differentSeed=ProceduralDesigner.Generate("x",0,new(basis,MacroContext.Neutral),intent with{Seed=43});
var seeded=ProceduralDesigner.Generate("x",0,new(basis,MacroContext.Neutral),intent);
Check(differentSeed.Zip(seeded).Any(pair=>!PatchCodec.ContentEquals(pair.First.After.Patch,pair.Second.After.Patch)),"Seeds produce distinct variation sets");
var zeroVariation=ProceduralDesigner.Generate("x",0,new(basis,MacroContext.Neutral),intent with{VariationAmount=0});
Check(zeroVariation.All(d=>PatchCodec.ContentEquals(d.After.Patch,basis)),"Zero variation matches template exactly");
var zero=ProceduralDesigner.Refine("track:0",0,state,intent,[new(RefinementDimension.ReleaseLength,0)]);
Check(zero.IsNoOp,"Zero refinement does not force release level or change anything");
var dark=ProceduralDesigner.Refine("track:0",0,state,intent,[new(RefinementDimension.Brightness,-50)],locked);
Check(!dark.IsNoOp,"Darker changes active modulators");
for(int n=1;n<=6;n++)
{
    var b=basis.GetOperator(n);var a=dark.After.Patch.GetOperator(n);
    Check(a.Envelope==b.Envelope,"Brightness keeps envelopes");
    Check(a.Mode==b.Mode&&a.Coarse==b.Coarse&&a.Fine==b.Fine&&a.Detune==b.Detune,"Brightness keeps tuning");
    if(AlgorithmTopology.IsCarrier(basis.Algorithm,n)||n==2)Check(a==b,"Carriers and locked operator unchanged");
    else Check(a.OutputLevel<b.OutputLevel,"Modulator quieter");
}
var organ=FactoryLibrary.Get(6);
Check(ProceduralDesigner.Refine("x",0,new(organ,MacroContext.Neutral),intent,[new(RefinementDimension.Brightness,50)]).IsNoOp,"All-carrier brightness safely no-ops");
var conflict=ProceduralDesigner.Refine("track:0",0,state,intent,[new(RefinementDimension.Brightness,50),new(RefinementDimension.Brightness,-25)]);
Check(conflict.IsNoOp&&conflict.Explanation.Any(x=>x.Contains("Conflicting")),"Conflict disclosed");
Reject(()=>ProceduralDesigner.Refine("x",0,state,intent,[new(RefinementDimension.Brightness,101)]),"Invalid edit");
Reject(()=>ProceduralDesigner.Generate("x",0,state,intent with{VariationAmount=-1}),"Invalid intent");
Reject(()=>ProceduralDesigner.Generate("x",0,state,intent,new(LockGroup.None,ImmutableHashSet.Create(7))),"Invalid lock");
Reject(()=>ProceduralDesigner.Generate("x",0,state with{Macros=new(33)},intent),"Invalid macros");
using(var cts=new CancellationTokenSource())
{
    cts.Cancel();bool canceled=false;
    try{ProceduralDesigner.Generate("x",0,state,intent,cancellationToken:cts.Token);}catch(OperationCanceledException){canceled=true;}
    Check(canceled,"Cancellation");
}
var original=PatchCodec.EncodeVoice(basis);
var doc=new SoundDocument("track:0",state);
var refined=ProceduralDesigner.RefineDraft(dark,[new(RefinementDimension.DecayLength,-20)]);
Equal(refined.Before.Patch,basis,"Follow-up retains original undo snapshot");
Check(refined.ParentDraftIdentity==dark.Identity&&refined.Refinements.Length==2,"Follow-up provenance");
doc.Apply(refined);Equal(doc.State.Patch,refined.After.Patch,"Complete apply");
Check(doc.Undo(),"Undo exists");Equal(doc.State.Patch,basis,"Complete undo patch");Check(doc.State.Macros==state.Macros,"Complete undo macros");
Check(doc.Redo(),"Redo exists");Equal(doc.State.Patch,refined.After.Patch,"Redo replays stored result");
Reject(()=>doc.Apply(dark),"Stale revision rejected");
Reject(()=>new SoundDocument("other",state).Apply(dark),"Wrong selection rejected");
var forged=dark with{After=new(basis.WithOperator(2,basis.GetOperator(2) with{OutputLevel=1}),state.Macros)};
Reject(()=>new SoundDocument("track:0",state).Apply(forged),"Apply independently enforces locks");
Check(PatchCodec.EncodeVoice(basis).SequenceEqual(original),"Generation never mutates source");
Check(doc.AcceptedHistory.Length==1,"History stores accepted proposal");
var parsed=PhraseParser.Parse("Make this patch darker but keep its attack and tuning",SoundFamily.Keys,42);
foreach(var negative in new[]{"not brighter","don't make it brighter","darker but not shorter","without brighter attack","never brighter"}) {
    var rejected=PhraseParser.Parse(negative,SoundFamily.Keys,42);
    Check(!rejected.HasSupportedRequest && rejected.Refinements.IsEmpty && rejected.Intent.Brightness==Character.Template,"Negation cannot invert a request");
}
Check(parsed.Locks.Groups==(LockGroup.Attack|LockGroup.Tuning),"Prompt preservation");
Check(parsed.Refinements.Single()==new Refinement(RefinementDimension.Brightness,-50),"Prompt darker");
Check(parsed.Notices.IsEmpty,"Known prompt fully understood");
parsed=PhraseParser.Parse("brighter and darker",SoundFamily.Keys,42);
Check(parsed.Intent.Brightness==Character.Template&&parsed.Notices.Any(x=>x.Contains("Conflicting")),"Parser conflict");
Check(!parsed.HasSupportedRequest && parsed.Refinements.IsEmpty,"Conflict cannot leak contradictory refinements");
foreach(var unsupported in new[]{"darker with convolution reverb", "brighter but keep the bass", "bass and bell", "darker by 10 percent"}) {
    var rejected=PhraseParser.Parse(unsupported,SoundFamily.Keys,42);
    Check(!rejected.HasSupportedRequest && rejected.Refinements.IsEmpty && rejected.Locks==PatchLocks.None,"Unsupported clause rejects whole sound request");
}
parsed=PhraseParser.Parse("turn it into a spaceship with convolution reverb",SoundFamily.Effect,42);
Check(!parsed.HasSupportedRequest&&parsed.Notices.Any(x=>x.Contains("Unsupported")),"Unsupported capabilities disclosed");
parsed=PhraseParser.Parse("A glassy bell that becomes brighter when I play harder",SoundFamily.Keys,42);
Check(parsed.Intent.Family==SoundFamily.Bell&&parsed.Intent.VelocityResponse==Character.High,"Longest phrase recognition");
var first=ProceduralDesigner.Generate("x",0,new(basis,MacroContext.Neutral),intent)[0];
Check(Convert.ToHexString(SHA256.HashData(PatchCodec.Pack(first.After.Patch)))==
    "2F9A0F17D73C5AC75A2A56A462108BA97CF9B8792DA6797B403DA3FC15025A40","Versioned recipe golden");
Console.WriteLine($"PASS: {assertions} assertions; firmware/web factory vectors, all bounds/topologies, SysEx, recipes, locks, parser, cancellation, stale apply, undo/redo.");
}
catch(Exception error)
{
    Console.Error.WriteLine($"FAIL: {error.Message}\n{error.StackTrace}");
    Environment.ExitCode=1;
}
