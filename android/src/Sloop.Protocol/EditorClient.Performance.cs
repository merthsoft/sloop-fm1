namespace Sloop.Protocol;

public sealed partial class EditorClient
{
    public async Task<PerformanceCapabilities?> DiscoverPerformanceAsync(int protocolVersion, CancellationToken token = default)
    {
        if (protocolVersion < 12) return null;
        var capabilities = PerformanceWire.ParseCapabilities(await RequestAsync(73, [0], cancellationToken: token).ConfigureAwait(false));
        return capabilities.Supported ? capabilities : null;
    }
    public async Task<PerformanceStatus> PressHardwareGestureAsync(uint owner, HardwareGestureKind kind, byte value = 0, CancellationToken token = default) =>
        PerformanceWire.ParseReply(await RequestAsync(73, PerformanceWire.Press(owner, kind, value), cancellationToken: token).ConfigureAwait(false), 1, owner);
    public async Task<PerformanceStatus> ReleaseHardwareGestureAsync(uint owner, CancellationToken token = default) =>
        PerformanceWire.ParseReply(await RequestAsync(73, [2, .. PerformanceWire.Token(owner)], cancellationToken: token).ConfigureAwait(false), 2, owner);
    public async Task<PerformanceStatus> RenewHardwareGestureAsync(uint owner, CancellationToken token = default) =>
        PerformanceWire.ParseReply(await RequestAsync(73, [3, .. PerformanceWire.Token(owner)], cancellationToken: token).ConfigureAwait(false), 3, owner);
    public async Task ClearHardwareGesturesAsync(CancellationToken token = default)
    {
        var status = PerformanceWire.ParseReply(await RequestAsync(73, [4], cancellationToken: token).ConfigureAwait(false), 4, 0);
        if (status != PerformanceStatus.Ok) throw new IOException("Hardware gesture cleanup rejected.");
    }
}
