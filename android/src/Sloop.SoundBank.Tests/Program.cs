using Sloop.Protocol;
using Sloop.SoundDesign;
using Sloop.Workstation;

int checks=0;
void Check(bool ok,string name) {checks++;if(!ok)throw new Exception(name);}
void Reject(Action action,string name) {try{action();}catch{checks++;return;}throw new Exception(name);}
var patch=FactoryLibrary.Get(0);var bytes=PatchCodec.Pack(patch);
Check(EditorClient.DecodeSoundBankRead(new(68,[1,0,2]),0) is null,"empty");
Check(EditorClient.DecodeSoundBankRead(new(68,[1,0,0,..bytes]),0)!.SequenceEqual(bytes),"packed read");
Reject(()=>EditorClient.DecodeSoundBankRead(new(68,[1,0,0,1]),0),"truncated");
Reject(()=>EditorClient.DecodeSoundBankRead(new(68,[0,0,2]),0),"target");
Reject(()=>EditorClient.DecodeSoundBankRead(new(68,[1,0,2,0]),0),"empty trailing");
var inventory=new List<byte>{8,27};for(int i=0;i<35;i++)inventory.AddRange([0,0]);
Check(EditorClient.DecodeSoundBankList(new(70,inventory.ToArray())).Count==27,"inventory");
Reject(()=>EditorClient.DecodeSoundBankList(new(70,inventory.SkipLast(1).ToArray())),"inventory truncated");
inventory[2]=2;Reject(()=>EditorClient.DecodeSoundBankList(new(70,inventory.ToArray())),"invalid used flag");
Reject(()=>SoundBankSavePlan.Create(27,null,patch,"name"),"slot bounds");
Reject(()=>SoundBankSavePlan.Create(0,null,patch,"é"),"name ascii");
var corrupt=bytes.ToArray();corrupt[0]=127;Reject(()=>SoundBankSavePlan.Create(0,corrupt,patch,"name"),"corrupt ranges");
async Task Run(string name,byte[]? before,bool confirmed,Func<Task<byte[]?>> read,Func<byte[],Task> write,Action guard,SoundBankSaveOutcome outcome) {
    var result=await SoundBankSave.Execute(SoundBankSavePlan.Create(0,before,patch,"SAVED"),confirmed,read,write,guard);
    Check(result.Outcome==outcome,name);
}
int writes=0;byte[]? bank=null;
await Run("empty save verified",null,false,()=>Task.FromResult(bank),p=>{writes++;bank=p;return Task.CompletedTask;},()=>{},SoundBankSaveOutcome.Verified);
Check(writes==1,"one write");
await Run("overwrite cancellation",bank,false,()=>throw new Exception("must not read"),p=>throw new Exception("must not write"),()=>{},SoundBankSaveOutcome.Refused);
await Run("conflicting destination",null,true,()=>Task.FromResult(bank),p=>throw new Exception("must not write"),()=>{},SoundBankSaveOutcome.Conflict);
foreach(string change in new[]{"track change","local revision","A/B ownership","connection epoch"})
    await Run(change,null,true,()=>Task.FromResult<byte[]?>(null),p=>throw new Exception("must not write"),()=>throw new IOException(change),SoundBankSaveOutcome.Conflict);
foreach(byte rc in new byte[]{2,3})
    await Run("flash refusal",null,true,()=>Task.FromResult<byte[]?>(null),p=>throw new SoundBankRefusal(rc),()=>{},SoundBankSaveOutcome.Refused);
await Run("disconnect during save",null,true,()=>Task.FromResult<byte[]?>(null),p=>throw new IOException("Disconnected after send"),()=>{},SoundBankSaveOutcome.Unknown);
await Run("ack lost",null,true,()=>Task.FromResult<byte[]?>(null),p=>throw new TimeoutException(),()=>{},SoundBankSaveOutcome.Unknown);
await Run("readback mismatch",null,true,()=>Task.FromResult<byte[]?>(null),p=>Task.CompletedTask,()=>{},SoundBankSaveOutcome.Unknown);
int reads=0;
await Run("verified overwrite",bytes,true,()=>Task.FromResult<byte[]?>(reads++==0?bytes:bank),p=>{bank=p;return Task.CompletedTask;},()=>{},SoundBankSaveOutcome.Verified);
Console.WriteLine($"Sound bank: {checks} checks passed.");
