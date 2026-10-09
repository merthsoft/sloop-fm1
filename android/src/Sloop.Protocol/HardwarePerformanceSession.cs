namespace Sloop.Protocol;

/// <summary>Serializes touches and renewals. A failed epoch is forgotten and never replayed.</summary>
public sealed class HardwarePerformanceSession(EditorClient client, PerformanceCapabilities capabilities)
{
    readonly SemaphoreSlim gate = new(1);
    readonly Dictionary<int, uint> owners = [];
    uint next;
    bool failed;
    public bool Available => capabilities.Supported && !failed;
    public async Task PressAsync(int pointer, HardwareGestureKind kind, byte value = 0, CancellationToken cancellation = default)
    {
        await gate.WaitAsync(cancellation).ConfigureAwait(false);
        try
        {
            cancellation.ThrowIfCancellationRequested();
            if (!Available) throw new NotSupportedException("FM1 hardware performance is unavailable.");
            if (owners.ContainsKey(pointer)) return;
            if (owners.Count >= capabilities.Owners || next == 0xfffffff) throw new InvalidOperationException("Hardware gesture owner limit.");
            uint token = ++next;
            // Never cancel a wire request halfway through a press: reconcile then release.
            var status = await client.PressHardwareGestureAsync(token, kind, value).ConfigureAwait(false);
            if (status != PerformanceStatus.Ok) throw new IOException($"Hardware gesture rejected: {status}.");
            owners.Add(pointer, token);
            if (cancellation.IsCancellationRequested)
            {
                await client.ReleaseHardwareGestureAsync(token).ConfigureAwait(false);
                owners.Remove(pointer);
                cancellation.ThrowIfCancellationRequested();
            }
        }
        catch (OperationCanceledException) { throw; }
        catch { failed = true; owners.Clear(); throw; }
        finally { gate.Release(); }
    }
    public async Task ReleaseAsync(int pointer)
    {
        await gate.WaitAsync().ConfigureAwait(false);
        try
        {
            if (owners.Remove(pointer, out var token) && !failed)
                await client.ReleaseHardwareGestureAsync(token).ConfigureAwait(false);
        }
        catch { failed = true; owners.Clear(); throw; }
        finally { gate.Release(); }
    }
    public async Task RenewAsync()
    {
        await gate.WaitAsync().ConfigureAwait(false);
        try
        {
            if (!Available) return;
            foreach (var owner in owners.ToArray())
            {
                var status = await client.RenewHardwareGestureAsync(owner.Value).ConfigureAwait(false);
                if (status == PerformanceStatus.Missing) owners.Remove(owner.Key); // STOP/expired/consumed next-bar
                else if (status != PerformanceStatus.Ok) throw new IOException("Hardware renewal rejected.");
            }
        }
        catch { failed = true; owners.Clear(); throw; }
        finally { gate.Release(); }
    }
    public async Task ClearAsync()
    {
        await gate.WaitAsync().ConfigureAwait(false);
        try { owners.Clear(); if (Available) await client.ClearHardwareGesturesAsync().ConfigureAwait(false); }
        catch { failed = true; throw; }
        finally { gate.Release(); }
    }
}
