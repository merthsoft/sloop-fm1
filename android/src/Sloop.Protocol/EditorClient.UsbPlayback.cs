namespace Sloop.Protocol;

[Flags]
public enum UsbPlaybackFeatures : byte { None=0, Gain=1, Mute=2, Diagnostics=4, Reset=8 }
public sealed record UsbPlaybackCapabilities(byte Schema, UsbPlaybackFeatures Features, int MaximumGainQ12, int RampFrames)
{
    public static UsbPlaybackCapabilities Unavailable { get; } = new(0,0,0,0);
    public bool Supported => Schema==1 && (Features & (UsbPlaybackFeatures.Gain|UsbPlaybackFeatures.Mute)) == (UsbPlaybackFeatures.Gain|UsbPlaybackFeatures.Mute);
}
public sealed record UsbPlaybackControl(int GainQ12, bool Muted);
/// <summary>Counters wrap modulo 2^32; reset changes counter baselines only. Fill extrema are lifetime observations.
/// Values are individually sampled, not an atomic multi-counter instant. AlternateSelected is not route confirmation.</summary>
public sealed record UsbPlaybackDiagnostics(uint Packets, uint Frames, uint Malformed, uint Overruns, uint Underruns,
    uint HardwareErrors, uint Starts, uint ClippedFrames, uint FillMinimum, uint FillMaximum, uint FillCurrent, uint RateQ16, bool AlternateSelected);

public sealed partial class EditorClient
{
    public async Task<UsbPlaybackCapabilities> GetUsbPlaybackCapabilitiesAsync(DeviceInfo info, CancellationToken token=default)
    {
        // Unknown commands on older firmware need not reply: never probe them and fault the MIDI epoch.
        if(info.ProtocolVersion<12) return UsbPlaybackCapabilities.Unavailable;
        var a=UsbPlaybackReply(await RequestAsync(75,[0],cancellationToken:token).ConfigureAwait(false),0,8);
        if(a[2]!=1) return UsbPlaybackCapabilities.Unavailable;
        if((a[3]&~15)!=0 || UsbU14(a,4)!=4096 || UsbU14(a,6)!=256) throw new FormatException("Invalid USB playback capabilities.");
        return new(a[2],(UsbPlaybackFeatures)a[3],UsbU14(a,4),UsbU14(a,6));
    }
    public async Task<UsbPlaybackControl> UsbPlaybackControlAsync(UsbPlaybackCapabilities capabilities, UsbPlaybackControl? value=null, CancellationToken token=default)
    {
        if(!capabilities.Supported) throw new NotSupportedException("FM1 USB return control unavailable.");
        if(value is not null && (value.GainQ12<0 || value.GainQ12>capabilities.MaximumGainQ12)) throw new ArgumentOutOfRangeException(nameof(value));
        byte[] args=value is null ? [1] : [1,(byte)(value.GainQ12&127),(byte)(value.GainQ12>>7),(byte)(value.Muted?1:0)];
        var a=UsbPlaybackReply(await RequestAsync(75,args,cancellationToken:token).ConfigureAwait(false),1,5);
        int gain=UsbU14(a,2);
        if(gain>capabilities.MaximumGainQ12 || a[4]>1) throw new FormatException("Invalid USB return state.");
        return new(gain,a[4]!=0);
    }
    public async Task<UsbPlaybackDiagnostics> GetUsbPlaybackDiagnosticsAsync(UsbPlaybackCapabilities capabilities, bool reset=false, CancellationToken token=default)
    {
        if(capabilities.Schema!=1 || !capabilities.Features.HasFlag(UsbPlaybackFeatures.Diagnostics) || (reset && !capabilities.Features.HasFlag(UsbPlaybackFeatures.Reset)))
            throw new NotSupportedException("FM1 USB playback diagnostics unavailable.");
        byte sub=(byte)(reset?3:2);
        var a=UsbPlaybackReply(await RequestAsync(75,[sub],cancellationToken:token).ConfigureAwait(false),sub,63);
        var v=new uint[12];
        for(int i=0;i<v.Length;i++) { int o=2+i*5; if(a[o+4]>15) throw new FormatException("Invalid USB diagnostic counter."); for(int j=0;j<5;j++) v[i]|=(uint)a[o+j]<<(7*j); }
        if(a[62]>1 || v[10]>1024 || v[8]>1024 || v[9]>1024 || v[11]<64855 || v[11]>66165) throw new FormatException("Invalid USB diagnostic state.");
        return new(v[0],v[1],v[2],v[3],v[4],v[5],v[6],v[7],v[8],v[9],v[10],v[11],a[62]!=0);
    }
    private static int UsbU14(byte[] a,int o)=>a[o]|a[o+1]<<7;
    private static byte[] UsbPlaybackReply(EditorFrame frame,byte sub,int length)
    {
        var a=frame.Arguments;
        if(frame.Command!=75 || a.Length<2 || a[0]!=sub || a.Any(v=>v>127)) throw new FormatException("Invalid USB playback reply.");
        if(a[1]==2 && a.Length==2) throw new NotSupportedException("FM1 USB playback unavailable.");
        if(a[1]!=0 || a.Length!=length) throw new FormatException("Rejected or malformed USB playback reply.");
        return a;
    }
}
