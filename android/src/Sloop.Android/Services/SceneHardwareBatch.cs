using System.Collections.Immutable;
using Sloop.SoundDesign;

namespace Sloop.Android.Services;

public sealed record SceneTrackSound(int Track, SoundState State, int Preset, int Selector);
public sealed class SceneHardwareBaseline
{
    internal object Connection { get; }
    public ImmutableArray<SceneTrackSound> Tracks { get; }
    internal SceneHardwareBaseline(object connection, IEnumerable<SceneTrackSound> tracks)
    { Connection=connection; Tracks=tracks.ToImmutableArray(); }
}
public sealed record SceneHardwareTarget(int Track, SoundState State);
public enum SceneHardwareOutcome { NotApplied, Applied, Partial, Unknown }
public sealed record SceneHardwareTrackResult(int Track, SceneHardwareOutcome Outcome,
    SceneTrackSound? Observed, string? Error);
public sealed record SceneHardwareResult(ImmutableArray<SceneHardwareTrackResult> Tracks)
{
    public bool Applied => Tracks.Length>0 && Tracks.All(t=>t.Outcome==SceneHardwareOutcome.Applied);
}

// Pure orchestration shared by the physical adapter and executable fault-injection tests.
internal static class SceneHardwareBatch
{
    internal static bool Same(SceneTrackSound a, SceneTrackSound b) => a.Track==b.Track &&
        a.Preset==b.Preset && a.Selector==b.Selector && a.State.Macros==b.State.Macros &&
        PatchCodec.ContentEquals(a.State.Patch,b.State.Patch);
    internal static async Task<SceneHardwareResult> Apply(
        IReadOnlyList<SceneTrackSound> baseline, IReadOnlyList<SceneHardwareTarget> targets,
        Func<int,Task<SceneTrackSound>> read, Func<SceneHardwareTarget,Task> write, Action guard, Action? checkCancellation=null)
    {
        var observed=new Dictionary<int,SceneTrackSound>();
        var attempted=new HashSet<int>(); string? error=null;
        try {
            // No write until every complete baseline has been checked.
            foreach(var b in baseline) { guard(); checkCancellation?.Invoke(); var fresh=await read(b.Track); observed[b.Track]=fresh;
                if(!Same(b,fresh))throw new IOException("Hardware sound, preset or PTCH changed since batch Read."); }
            foreach(var target in targets) {
                guard(); checkCancellation?.Invoke(); var fresh=await read(target.Track); observed[target.Track]=fresh;
                if(!Same(baseline.Single(b=>b.Track==target.Track),fresh))
                    throw new IOException("Hardware baseline changed before track write.");
                guard(); checkCancellation?.Invoke(); attempted.Add(target.Track); await write(target);
                guard(); var verified=await read(target.Track);
                if(!Same(baseline.Single(b=>b.Track==target.Track) with {State=target.State},verified))
                    throw new IOException("Complete sound readback differs after track write.");
            }
        } catch(Exception e) { error=e.Message; }
        // Reconcile all tracks after any attempted write. Never use a pre-write observation
        // as evidence of post-write state; read failure leaves the track Unknown.
        if(attempted.Count>0) {
            observed.Clear();
            foreach(var b in baseline)try { guard(); observed[b.Track]=await read(b.Track); }
                catch(Exception e) { error ??= e.Message; }
        }
        return new(targets.Select(target=> {
            observed.TryGetValue(target.Track,out var actual);
            var b=baseline.Single(b=>b.Track==target.Track);
            var desired=b with { State=target.State };
            var outcome=attempted.Count==0?SceneHardwareOutcome.NotApplied:
                actual is null?SceneHardwareOutcome.Unknown:
                Same(actual,desired)?SceneHardwareOutcome.Applied:
                attempted.Contains(target.Track)||!Same(actual,b)?SceneHardwareOutcome.Partial:SceneHardwareOutcome.NotApplied;
            return new SceneHardwareTrackResult(target.Track,outcome,actual,error);
        }).ToImmutableArray());
    }
}
