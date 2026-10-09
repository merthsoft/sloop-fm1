using Sloop.Protocol;
using Sloop.SoundDesign;
using Sloop.Workstation;

namespace Sloop.Android.Services;
public sealed record PreparedSoundBankSave(SoundBankSavePlan Plan,object Connection);
public sealed partial class Fm1Connection
{
    public Task<Patch?> ReadSoundBankAsync(byte slot)=>EditDeviceAsync("Inspecting FM6 bank voice…",async(c,t)=> {
        ValidateSceneTracks([0]);var bytes=await c.ReadSoundBankAsync(slot,t);return bytes is null?null:PatchCodec.Unpack(bytes);
    });
    public Task<IReadOnlyList<SoundBankEntry>> ListSoundBankAsync()=>EditDeviceAsync("Reading FM6 bank…",(c,t)=> {
        ValidateSceneTracks([0]); return c.ListSoundBankAsync(t);
    });
    public Task<PreparedSoundBankSave> PrepareSoundBankSaveAsync(byte slot,Patch patch,string name)=>
        EditDeviceAsync("Reading bank destination…",async(c,t)=> {
            ValidateSceneTracks([0]); var previous=await c.ReadSoundBankAsync(slot,t);
            return new PreparedSoundBankSave(SoundBankSavePlan.Create(slot,previous,patch,name),epoch!);
        });
    public async Task<SoundBankSaveResult> SaveSoundBankAsync(PreparedSoundBankSave prepared,bool overwriteConfirmed,Action guard)
    {
        return await EditDeviceAsync("Saving and verifying FM6 flash…",async(c,t)=> {
            void Check() {
                t.ThrowIfCancellationRequested();guard();
                if(!ReferenceEquals(epoch,prepared.Connection)||IsPlaying||IsRecording||Performer.Active)
                    throw new IOException("Connection or transport changed since bank confirmation.");
            }
            return await SoundBankSave.Execute(prepared.Plan,overwriteConfirmed,
                ()=>c.ReadSoundBankAsync(prepared.Plan.Slot,t),p=>c.WriteSoundBankAsync(prepared.Plan.Slot,p,t),Check);
        });
    }
}
