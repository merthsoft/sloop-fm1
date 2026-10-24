using Sloop.Protocol;
namespace Sloop.Android.Services;

public sealed partial class Fm1Connection
{
    HardwarePerformanceSession? hardwarePerformance;
    CancellationTokenSource? hardwareRenewal;
    Task? hardwareRenewalTask;
    Task hardwareRetirement = Task.CompletedTask;
    EditorClient? hardwareEditor;
    int hardwareGeneration;
    public byte HardwareEffectCount => hardwarePerformance?.EffectCount ?? 0;
    public bool CanPerformHardware => !Snapshot.Busy && !Snapshot.IsSimulated && !Snapshot.IsGenericMidi && hardwarePerformance?.Available == true;
    public async Task InitializeHardwarePerformanceAsync(EditorClient editor, CancellationToken token)
    {
        if(token.IsCancellationRequested || client != editor) return;
        // Firmware capabilities are immutable within an editor epoch; song refresh needs no new owner.
        if(hardwareEditor == editor && hardwarePerformance?.Available == true && hardwareRenewal?.IsCancellationRequested == false) return;
        int generation = ++hardwareGeneration;
        await RetireHardwarePerformanceAsync();
        if(generation != hardwareGeneration || token.IsCancellationRequested || client != editor) return;
        var capabilities = await editor.DiscoverPerformanceAsync(Snapshot.Device?.ProtocolVersion ?? 0, token);
        if (generation != hardwareGeneration || token.IsCancellationRequested || client != editor || capabilities is null || Snapshot.IsSimulated || Snapshot.IsGenericMidi) return;
        hardwarePerformance = new(editor, capabilities);
        hardwareEditor = editor;
        hardwareRenewal = CancellationTokenSource.CreateLinkedTokenSource(token);
        hardwareRenewalTask = RenewHardwarePerformanceAsync(hardwarePerformance, hardwareRenewal.Token);
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
        ++hardwareGeneration;
        await RetireHardwarePerformanceAsync();
    }
    Task RetireHardwarePerformanceAsync()
    {
        var renewal = hardwareRenewal; var task = hardwareRenewalTask;
        var session = hardwarePerformance;
        hardwareRenewal = null; hardwareRenewalTask = null; hardwarePerformance = null; hardwareEditor = null;
        renewal?.Cancel();
        // A second initializer must also await a retirement whose fields are already detached.
        return hardwareRetirement = FinishRetirementAsync(hardwareRetirement,task,renewal,session);
    }
    static async Task FinishRetirementAsync(Task previous,Task? task,CancellationTokenSource? renewal,HardwarePerformanceSession? session)
    {
        await previous;
        previous = Task.CompletedTask; // Do not retain a chain of completed retirements across reconnects.
        if(task is not null) await task;
        renewal?.Dispose();
        if (session is not null) try { await session.ClearAsync(); } catch (Exception) { }
    }
}
