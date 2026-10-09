using Sloop.Protocol;
using Sloop.SoundDesign;

namespace Sloop.Workstation;

public enum SoundBankSaveOutcome { Verified, Refused, Conflict, Unknown }
public sealed record SoundBankSaveResult(SoundBankSaveOutcome Outcome,string Message);
public sealed record SoundBankSavePlan(byte Slot,byte[]? Before,byte[] Desired)
{
    public static SoundBankSavePlan Create(byte slot,byte[]? before,Patch patch,string name)
    {
        if(slot>=27)throw new ArgumentOutOfRangeException(nameof(slot));
        if(string.IsNullOrWhiteSpace(name)||name.Length>10||name.Any(c=>c<32||c>126))throw new ArgumentException("Enter 1–10 printable ASCII characters.");
        if(before is not null)_=PatchCodec.Unpack(before);
        return new(slot,before?.ToArray(),PatchCodec.Pack(patch with{Name=name}));
    }
}
public static class SoundBankSave
{
    public static async Task<SoundBankSaveResult> Execute(SoundBankSavePlan plan,bool overwriteConfirmed,
        Func<Task<byte[]?>> read,Func<byte[],Task> write,Action guard)
    {
        bool attempted=false;
        try {
            if(plan.Before is not null&&!overwriteConfirmed)return new(SoundBankSaveOutcome.Refused,"Overwrite cancelled; bank unchanged.");
            guard(); var fresh=await read(); guard();
            if(!Same(plan.Before,fresh))return new(SoundBankSaveOutcome.Conflict,"Destination changed since confirmation. Refresh the bank.");
            attempted=true; await write(plan.Desired); guard();
            var actual=await read(); guard();
            if(!Same(actual,plan.Desired))return new(SoundBankSaveOutcome.Unknown,"Save acknowledged but readback differs. Reconnect and inspect this slot; do not retry automatically.");
            return new(SoundBankSaveOutcome.Verified,$"B{plan.Slot+1} base voice saved and verified in FM1 flash. Track macros and project are separate.");
        } catch(SoundBankRefusal e) {return new(SoundBankSaveOutcome.Refused,e.Message);}
        catch(Exception e) {return new(attempted?SoundBankSaveOutcome.Unknown:SoundBankSaveOutcome.Conflict,
            (attempted?"Persistence uncertain. Reconnect and inspect the destination before another save. ":"Save cancelled before write. ")+e.Message);}
    }
    static bool Same(byte[]? a,byte[]? b)=>a is null?b is null:b is not null&&a.AsSpan().SequenceEqual(b);
}
