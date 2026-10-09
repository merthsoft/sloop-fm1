namespace Sloop.Workstation;

/// <summary>Owns only the transport started by a draft preview. Calls belong to the UI thread.</summary>
public sealed class CompositionAudition(Func<Guid?> currentEpoch, Action stopPlayback)
{
    Guid? ownedEpoch;

    public async Task RunAsync(Func<Task> startPlayback)
    {
        var playback = startPlayback();
        // A rejected start must not acquire an already-running transport.
        if (playback.IsCompleted) { await playback; return; }
        var owned = currentEpoch();
        ownedEpoch = owned;
        try { await playback; }
        finally { if (ownedEpoch == owned) ownedEpoch = null; }
    }

    public void Stop()
    {
        var owned = ownedEpoch;
        // Clear before callbacks: a stop notification may re-enter cleanup.
        ownedEpoch = null;
        if (owned is not null && currentEpoch() == owned) stopPlayback();
    }
}
