using System.Collections.Immutable;
using Sloop.Sequencing;

namespace Sloop.Scenes;
public static class SceneEditing
{
    public static SceneSnapshot Capture(string name, int tempo, IEnumerable<SceneSound> sounds,
        AppPattern? pattern = null, IEnumerable<NativePatternIdentity>? nativePatterns = null)
    {
        var scene = new SceneSnapshot(Guid.NewGuid(), Guid.NewGuid(), name, tempo,
            sounds.ToImmutableArray(), pattern, nativePatterns?.ToImmutableArray() ?? []);
        scene.Validate(); return scene;
    }
    public static SceneSnapshot Replace(SceneSnapshot current, Guid expectedRevision, SceneSnapshot replacement)
    {
        if (current.Revision != expectedRevision || replacement.Id != current.Id)
            throw new InvalidOperationException("Stale scene edit or changed identity.");
        var next = replacement with { Revision = Guid.NewGuid() }; next.Validate(); return next;
    }
    public static Arrangement Move(Arrangement arrangement, int from, int to, IReadOnlyDictionary<Guid, SceneSnapshot> scenes)
    {
        arrangement.Validate(scenes);
        if (from < 0 || to < 0 || from >= arrangement.Steps.Length || to >= arrangement.Steps.Length)
            throw new ArgumentOutOfRangeException(nameof(from));
        var steps = arrangement.Steps.ToList(); var step = steps[from]; steps.RemoveAt(from); steps.Insert(to,step);
        return arrangement with { Steps = steps.ToImmutableArray() };
    }
}
