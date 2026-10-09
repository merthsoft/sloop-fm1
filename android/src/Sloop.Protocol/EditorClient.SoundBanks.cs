namespace Sloop.Protocol;

public sealed record SoundBankEntry(byte Slot, bool Used, string Name);
public sealed class SoundBankRefusal(byte status) : IOException(status switch {
    1 => "Invalid FM6 bank destination.", 2 => "FM6 flash refused the operation (flash failure or backup busy).",
    3 => "Stop the device song before writing flash.", _ => "Unknown FM6 bank status."
}) { public byte Status { get; } = status; }

public sealed partial class EditorClient
{
    public static IReadOnlyList<SoundBankEntry> DecodeSoundBankList(EditorFrame frame)
    {
        if(frame.Command!=70)throw new FormatException("Expected FM6 inventory.");
        var r=new PayloadReader(frame.Arguments);
        if(r.Byte()!=8||r.Byte()!=27)throw new FormatException("Unsupported FM6 bank schema.");
        var entries=new List<SoundBankEntry>();
        for(int i=0;i<35;i++) {
            byte used=r.Byte(); var name=r.String();
            if(used>1||name.Length>10||name.Any(c=>c<32||c>126)||used==0&&name.Length!=0)
                throw new FormatException("Corrupt FM6 inventory entry.");
            if(i>=8)entries.Add(new((byte)(i-8),used==1,name));
        }
        if(r.Remaining!=0)throw new FormatException("Trailing FM6 inventory data.");
        return entries.AsReadOnly();
    }
    public async Task<IReadOnlyList<SoundBankEntry>> ListSoundBankAsync(CancellationToken token=default)=>
        DecodeSoundBankList(await RequestAsync(70,[],cancellationToken:token));
    static void BankSlot(byte slot) { if(slot>=27)throw new ArgumentOutOfRangeException(nameof(slot)); }
    public static byte[]? DecodeSoundBankRead(EditorFrame frame,byte slot)
    {
        BankSlot(slot); var a=frame.Arguments;
        if(frame.Command!=68||a.Length<3||a[0]!=1||a[1]!=slot)throw new FormatException("Invalid bank read reply.");
        if(a[2]==2&&a.Length==3)return null;
        if(a[2]!=0) {if(a.Length!=3)throw new FormatException("Invalid bank status length.");throw new SoundBankRefusal(a[2]);}
        if(a.Length!=131||a.AsSpan(3).ContainsAnyInRange((byte)128,(byte)255))throw new FormatException("Invalid packed FM6 bank record.");
        return a[3..];
    }
    public async Task<byte[]?> ReadSoundBankAsync(byte slot,CancellationToken token=default)
    {
        BankSlot(slot);return DecodeSoundBankRead(await RequestAsync(68,[1,slot],cancellationToken:token),slot);
    }
    public async Task WriteSoundBankAsync(byte slot,byte[] packed,CancellationToken token=default)
    {
        BankSlot(slot);
        if(packed.Length!=128||packed.Any(b=>b>127))throw new ArgumentException("Expected 128 seven-bit FM6 bytes.");
        var r=await RequestAsync(69,[1,slot,..packed],timeout:TimeSpan.FromSeconds(10),cancellationToken:token);
        if(r.Arguments.Length!=3||r.Arguments[0]!=1||r.Arguments[1]!=slot)throw new FormatException("Invalid bank write reply.");
        if(r.Arguments[2]!=0)throw new SoundBankRefusal(r.Arguments[2]);
    }
}
