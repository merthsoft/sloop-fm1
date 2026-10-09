using Sloop.Protocol;
namespace Sloop.Android.Services;

public sealed partial class Fm1Connection
{
    public UsbPlaybackCapabilities UsbPlaybackCapabilities { get; private set; } = global::Sloop.Protocol.UsbPlaybackCapabilities.Unavailable;
    public UsbPlaybackControl? UsbPlaybackState { get; private set; }
    public UsbPlaybackDiagnostics? UsbPlaybackDiagnostics { get; private set; }
    // Parent calls after INFO and on Disconnect. All methods run on Android main thread.
    private void ResetUsbPlayback()
    {
        UsbPlaybackCapabilities=global::Sloop.Protocol.UsbPlaybackCapabilities.Unavailable;
        UsbPlaybackState=null; UsbPlaybackDiagnostics=null;
    }
    private async Task NegotiateUsbPlaybackAsync(EditorClient editor, DeviceInfo info, CancellationTokenSource current)
    {
        var caps=await editor.GetUsbPlaybackCapabilitiesAsync(info,current.Token);
        if(epoch!=current) return;
        UsbPlaybackCapabilities=caps;
        if(caps.Supported) {
            var state=await editor.UsbPlaybackControlAsync(caps,token:current.Token);
            if(epoch!=current) return;
            UsbPlaybackState=state;
        }
    }
    public async Task SetUsbPlaybackAsync(int gainQ12,bool muted)
    {
        var editor=client; var current=epoch;
        if(editor is null || current is null || !UsbPlaybackCapabilities.Supported) return;
        var state=await editor.UsbPlaybackControlAsync(UsbPlaybackCapabilities,new(gainQ12,muted),current.Token);
        if(epoch!=current) return;
        UsbPlaybackState=state; Changed?.Invoke();
    }
    public async Task RefreshUsbPlaybackDiagnosticsAsync(bool reset=false)
    {
        var editor=client;var current=epoch;
        if(editor is null || current is null || !UsbPlaybackCapabilities.Features.HasFlag(UsbPlaybackFeatures.Diagnostics)) return;
        var diagnostics=await editor.GetUsbPlaybackDiagnosticsAsync(UsbPlaybackCapabilities,reset,current.Token);
        if(epoch!=current) return;
        UsbPlaybackDiagnostics=diagnostics;Changed?.Invoke();
    }
}
