using Sloop.Sequencing;

namespace Sloop.Scenes;
public enum SceneBoundary { Beat, Bar, Phrase }
public enum SceneRestoreMode { AppOnly, HardwareReconciled, HardwareAtomic }
public enum RestoreOutcome { Applied, Partial, Unknown }
public sealed record MusicalPosition(Guid TransportEpoch, Tick Position, int TicksPerQuarter, int BeatsPerBar, Tick PhraseLength, Tick? PhraseOrigin = null)
{
    public void Validate()
    {
        if (TransportEpoch == Guid.Empty || Position.Value < 0 || TicksPerQuarter <= 0 || BeatsPerBar <= 0 || PhraseLength.Value <= 0)
            throw new ArgumentException("Invalid transport position.");
    }
    public long Quantum(SceneBoundary boundary) => boundary switch
    {
        SceneBoundary.Beat => TicksPerQuarter,
        SceneBoundary.Bar => checked((long)TicksPerQuarter * BeatsPerBar),
        SceneBoundary.Phrase => PhraseLength.Value,
        _ => throw new ArgumentException("Unknown boundary.")
    };
}
public sealed record PreparedScene(Guid Token, SceneSnapshot Scene, Guid ExpectedLiveRevision, SceneRestoreMode Mode);
public sealed record QueuedScene(PreparedScene Prepared, MusicalPosition Basis, Tick Due);
public sealed record SceneCommit(Guid Token, SceneSnapshot Scene, Guid BeforeRevision, Guid AfterRevision, SceneRestoreMode Mode);
public sealed record SceneReconciliation(Guid Token, RestoreOutcome Outcome, string Detail);
/// <summary>Single transport-thread owner. No I/O here; commit authorizes the adapter to restore.
/// App state must be published as one swap after owned notes are cleaned up. Device results require reconciliation.</summary>
public sealed class SceneScheduler
{
    public Guid LiveRevision { get; private set; }
    public PreparedScene? Prepared { get; private set; }
    public QueuedScene? Pending { get; private set; }
    public SceneCommit? AwaitingReconciliation { get; private set; }
    public SceneReconciliation? LastReconciliation { get; private set; }
    public SceneScheduler(Guid liveRevision)
    { if (liveRevision == Guid.Empty) throw new ArgumentException("Missing live revision."); LiveRevision = liveRevision; }
    public PreparedScene Prepare(SceneSnapshot scene, Guid expectedLiveRevision, SceneRestoreMode mode)
    {
        scene.Validate();
        if (!Enum.IsDefined(mode) || mode == SceneRestoreMode.HardwareAtomic)
            throw new NotSupportedException("FM1 multi-command scene restore is not atomic.");
        if (mode == SceneRestoreMode.AppOnly && !scene.NativePatterns.IsEmpty)
            throw new NotSupportedException("Native pattern references require a reconciled hardware restore.");
        if (expectedLiveRevision != LiveRevision || AwaitingReconciliation is not null)
            throw new InvalidOperationException("Stale live state or unresolved restore.");
        Pending = null;
        return Prepared = new(Guid.NewGuid(), scene, expectedLiveRevision, mode);
    }
    public QueuedScene Queue(Guid token, MusicalPosition position, SceneBoundary boundary)
    {
        position.Validate();
        var prepared = Require(token);
        var quantum = position.Quantum(boundary);
        // Always the next boundary: a request at a boundary cannot retroactively enter it.
        long origin=boundary==SceneBoundary.Phrase?position.PhraseOrigin?.Value??0:0;
        if(origin<0||origin>position.Position.Value)throw new ArgumentException("Invalid phrase origin.");
        var due = checked(origin+((position.Position.Value-origin) / quantum + 1) * quantum);
        return Pending = new(prepared, position, new(due));
    }
    public SceneCommit? Commit(MusicalPosition position)
    {
        position.Validate();
        if (Pending is not { } queue) return null;
        if (position.TransportEpoch != queue.Basis.TransportEpoch || position.TicksPerQuarter != queue.Basis.TicksPerQuarter ||
            position.BeatsPerBar != queue.Basis.BeatsPerBar || position.PhraseLength != queue.Basis.PhraseLength)
        { Cancel(queue.Prepared.Token); throw new InvalidOperationException("Transport basis changed; prepare and queue again."); }
        if (position.Position.Value < queue.Due.Value) return null;
        if (position.Position != queue.Due)
        { Cancel(queue.Prepared.Token); throw new InvalidOperationException("Missed boundary; requeue instead of applying late."); }
        var prepared = Require(queue.Prepared.Token);
        var commit = new SceneCommit(prepared.Token, prepared.Scene, LiveRevision, Guid.NewGuid(), prepared.Mode);
        Prepared = null; Pending = null; AwaitingReconciliation = commit;
        return commit;
    }
    public void Reconcile(SceneReconciliation result)
    {
        if (!Enum.IsDefined(result.Outcome) || AwaitingReconciliation is not { } commit || result.Token != commit.Token)
            throw new InvalidOperationException("No matching restore to reconcile.");
        LastReconciliation = result;
        // Even partial writes invalidate the prior live revision. Unknown state blocks further work.
        LiveRevision = commit.AfterRevision;
        if (result.Outcome != RestoreOutcome.Unknown) AwaitingReconciliation = null;
    }
    public bool Cancel(Guid token)
    {
        if (Prepared?.Token != token) return false;
        Prepared = null; Pending = null; return true;
    }
    public void ObserveLiveRevision(Guid revision)
    {
        if (revision == Guid.Empty || AwaitingReconciliation is not null) throw new InvalidOperationException("Reconcile restore first.");
        Prepared = null; Pending = null; LiveRevision = revision;
    }
    private PreparedScene Require(Guid token) => Prepared is { } p && p.Token == token && p.ExpectedLiveRevision == LiveRevision
        ? p : throw new InvalidOperationException("Missing or stale prepared scene.");
}
