using Sloop.Protocol;
using Sloop.Workstation;
namespace Sloop.Android.Services;

public sealed partial class Fm1Connection
{
    async Task RefreshPerformanceChannelsAsync(EditorClient editor,DeviceInfo info,CancellationToken token)
    {
        int[] channels=[0,1,2,9];
        for(byte track=0;track<4;track++) {
            if(track<3 && info.GlobalCount<45)continue;
            byte id=track==3?(byte)24:(byte)(33+track);
            var reply=await editor.RequestAsync(2,[1,id],r=>r.Arguments.Length==4 && r.Arguments[0]==1 && r.Arguments[1]==id,cancellationToken:token);
            int value=EditorCodec.DecodeValue(reply.Arguments.AsSpan(2));
            if(value is <0 or >16)throw new IOException("Invalid FM1 MIDI track channel.");
            channels[track]=value-1;
        }
        PerformanceChannels=Array.AsReadOnly(channels);
    }
    PerformancePlayer? performer;
    public PerformancePlayer Performer=>performer??=new(SendPerformance);
    public bool CanPerform=>!Snapshot.Busy&&!IsPlaying&&(Snapshot.Device is not null||Snapshot.IsGenericMidi)&&!Snapshot.IsSimulated&&transport is IMidiTransport;
    void SendPerformance(byte[] bytes)
    {
        // Offline mode keeps the playable visualization available without claiming audio.
        if((Snapshot.Device is null&&!Snapshot.IsGenericMidi)||Snapshot.IsSimulated)return;
        if(transport is not IMidiTransport midi)throw new IOException("The MIDI device is unavailable.");
        bool release=(bytes[0]&0xf0)==0x80;
        if(!release&&(Snapshot.Busy||IsPlaying))throw new IOException("Stop sequence playback and finish the device operation before performing.");
        midi.SendAsync(bytes,CancellationToken.None).GetAwaiter().GetResult();
    }
    public void ReleasePerformance()=>performer?.Panic();
    public async Task<string> CheckPerformanceAsync(int track,int channel)
    {
        return await EditDeviceAsync("Checking performance destination…",async(c,t)=> {
            var info=Snapshot.Device!;
            await RefreshPerformanceChannelsAsync(c,info,t);
            var reply=await c.RequestAsync(29,[(byte)track],cancellationToken:t);
            if(reply.Arguments.Length!=3+info.ParameterCount*2||reply.Arguments[0]!=track||!info.HasSupportedTrackLayout)throw new IOException("Unsupported parameter layout.");
            int Value(int id)=>EditorCodec.DecodeValue(reply.Arguments.AsSpan(3+id*2,2));
            async Task<int> Global(byte id){var f=await c.RequestAsync(2,[1,id],r=>r.Arguments.Length==4&&r.Arguments[0]==1&&r.Arguments[1]==id,cancellationToken:t);return EditorCodec.DecodeValue(f.Arguments.AsSpan(2));}
            var conflicts=new List<string>();
            if(track<3){if(Value(17)!=0)conflicts.Add("hardware arp");if(Value(49)!=0)conflicts.Add("hardware chord");if(Value(27)!=0)conflicts.Add("hardware quantize");if(Value(28)!=0)conflicts.Add("hardware transpose");}
            if(await Global(14)!=0)conflicts.Add("MIDI IN=CLOCK");
            int drum=await Global(24);if(track==3&&channel+1!=drum)conflicts.Add($"channel mismatch (FM1 drum channel {drum})");
            if(PerformanceChannels[track]<0)conflicts.Add("MIDI track channel is OFF");
            else if(channel!=PerformanceChannels[track])conflicts.Add($"channel mismatch (FM1 track channel {PerformanceChannels[track]+1}); refreshed app routing");
            if(track<3&&channel+1==drum)conflicts.Add("channel routing to drums instead of synth");
            return conflicts.Count==0?"Destination has no additional chord/arp/quantize/transpose transforms.":"Disable "+string.Join(", ",conflicts)+" on FM1 for the app's exact voicing. Settings were left unchanged.";
        });
    }
}
