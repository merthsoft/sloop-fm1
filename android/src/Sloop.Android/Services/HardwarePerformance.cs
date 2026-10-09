using Sloop.Protocol;
namespace Sloop.Android.Services;

public sealed partial class Fm1Connection
{
    HardwarePerformanceSession? hardwarePerformance;
    CancellationTokenSource? hardwareRenewal;
    public bool CanPerformHardware => !Snapshot.Busy && !Snapshot.IsSimulated && !Snapshot.IsGenericMidi && hardwarePerformance?.Available == true;
    public async Task InitializeHardwarePerformanceAsync(EditorClient editor, CancellationToken token)
    {
        var capabilities = await editor.DiscoverPerformanceAsync(Snapshot.Device?.ProtocolVersion ?? 0, token);
        if (token.IsCancellationRequested || client != editor || capabilities is null || Snapshot.IsSimulated || Snapshot.IsGenericMidi) return;
        hardwarePerformance = new(editor, capabilities);
        hardwareRenewal = CancellationTokenSource.CreateLinkedTokenSource(token);
        _ = RenewHardwarePerformanceAsync(hardwarePerformance, hardwareRenewal.Token);
        Changed?.Invoke();
    }
    async Task RenewHardwarePerformanceAsync(HardwarePerformanceSession session, CancellationToken token)
    {
        try
        {
            using var timer = new PeriodicTimer(TimeSpan.FromMilliseconds(200));
            while (await timer.WaitForNextTickAsync(token)) await session.RenewAsync();
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested) { }
        catch (Exception) { handler.Post(() => Changed?.Invoke()); }
    }
    public Task PressHardwareAsync(int pointer, HardwareGestureKind kind, byte value = 0) => CanPerformHardware
        ? hardwarePerformance!.PressAsync(pointer, kind, value) : Task.CompletedTask;
    public Task ReleaseHardwareAsync(int pointer) => hardwarePerformance?.ReleaseAsync(pointer) ?? Task.CompletedTask;
    public async Task ClearHardwarePerformanceAsync()
    {
        var session = hardwarePerformance;
        if (session is null) return;
        try { await session.ClearAsync(); }
        catch (Exception) { handler.Post(() => Changed?.Invoke()); } // failed session forgets owners; lease bounds device cleanup
    }
    // Parent calls before disposing the MIDI epoch; lease expiry is the fallback if transport fails.
    public async Task CloseHardwarePerformanceAsync()
    {
        hardwareRenewal?.Cancel(); hardwareRenewal?.Dispose(); hardwareRenewal = null;
        var session = hardwarePerformance; hardwarePerformance = null;
        if (session is not null) try { await session.ClearAsync(); } catch (Exception) { }
    }
}
