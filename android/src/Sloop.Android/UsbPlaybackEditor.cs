using Android.Widget;
using Sloop.Protocol;
namespace Sloop.Android;

public sealed partial class MainActivity
{
    // Call from the connected Perform/Sound page. Device routing remains in SampleWorkspace.
    void AddUsbPlaybackControls()
    {
        var caps=connection.UsbPlaybackCapabilities;
        if(!caps.Supported) { content.AddView(Label("FM1 USB return controls unavailable on this firmware.",12));return; }
        var state=connection.UsbPlaybackState;
        if(state is null) return;
        content.AddView(Label("USB phone return / instrument capture stays separate",12));
        var gain=new SeekBar(this){Max=caps.MaximumGainQ12,Progress=state.GainQ12,ContentDescription="USB return gain"};
        var mute=new CheckBox(this){Text="Mute USB return",Checked=state.Muted};
        var status=Label($"Return gain {state.GainQ12*100/4096}% / 256-frame fade",12);
        bool busy=false;
        async Task Save()
        {
            if(busy)return;busy=true;gain.Enabled=mute.Enabled=false;
            try { await connection.SetUsbPlaybackAsync(gain.Progress,mute.Checked); }
            catch(Exception error) { status.Text=error.Message; }
            finally { busy=false;gain.Enabled=mute.Enabled=true; }
        }
        gain.ProgressChanged+=(_,e)=>status.Text=$"Return gain {e.Progress*100/4096}% / 256-frame fade";
        gain.StopTrackingTouch+=async(_,_)=>await Save();
        mute.CheckedChange+=async(_,_)=>await Save();
        content.AddView(gain);content.AddView(mute);content.AddView(status);
        if(caps.Features.HasFlag(UsbPlaybackFeatures.Diagnostics)) {
            var diagnostics=Label("USB counters are sampled observations; output routing must be confirmed separately.",12);
            void ShowDiagnostics()
            {
                if(connection.UsbPlaybackDiagnostics is {} d) diagnostics.Text=$"Packets {d.Packets}; frames {d.Frames}; underruns {d.Underruns}; overruns {d.Overruns}\nMalformed {d.Malformed}; hardware errors {d.HardwareErrors}; clipped frames {d.ClippedFrames}\nFill {d.FillCurrent} ({d.FillMinimum} to {d.FillMaximum} lifetime); rate Q16 {d.RateQ16}";
            }
            ShowDiagnostics(); // Changed may rebuild this workspace; render the retained snapshot.
            async Task Read(bool reset) {
                try {
                    await connection.RefreshUsbPlaybackDiagnosticsAsync(reset);
                    ShowDiagnostics();
                } catch(Exception error) { diagnostics.Text=error.Message; }
            }
            var read=new Button(this){Text="Read USB diagnostics"};read.Click+=async(_,_)=>await Read(false);content.AddView(read);
            if(caps.Features.HasFlag(UsbPlaybackFeatures.Reset)) { var reset=new Button(this){Text="Reset USB counter baseline"};reset.Click+=async(_,_)=>await Read(true);content.AddView(reset); }
            content.AddView(diagnostics);
        }
    }
}
