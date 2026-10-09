using System.Collections.Immutable;

namespace Sloop.Scenes;

/// <summary>Catalog edits preserve references and validate automation against replacement patterns.</summary>
public static class CatalogEditing
{
    public static SceneCatalog Replace(SceneCatalog catalog, Guid id, Guid expectedRevision, SceneSnapshot replacement)
    {
        catalog.Validate();
        var current = catalog.Scenes.Single(s => s.Id == id);
        var next = SceneEditing.Replace(current, expectedRevision, replacement);
        var result = catalog with { Scenes = catalog.Scenes.Select(s => s.Id == id ? next : s).ToImmutableArray() };
        result.Validate();
        return result;
    }

    public static SceneCatalog Duplicate(SceneCatalog catalog, Guid id, string name)
    {
        catalog.Validate();
        var source = catalog.Scenes.Single(s => s.Id == id);
        var copy = source with { Id = Guid.NewGuid(), Revision = Guid.NewGuid(), Name = name };
        var result = catalog with {
            Scenes = catalog.Scenes.Add(copy),
            Automation = catalog.Automation.AddRange(catalog.Automation.Where(a => a.SceneId == id)
                .Select(a => new SceneAutomation(copy.Id, a.Lane with { Id = Guid.NewGuid() })))
        };
        result.Validate();
        return result;
    }

    public static SceneCatalog Delete(SceneCatalog catalog, Guid id, Guid expectedRevision)
    {
        catalog.Validate();
        var current = catalog.Scenes.Single(s => s.Id == id);
        if (current.Revision != expectedRevision) throw new InvalidOperationException("Scene changed since the delete request.");
        if (catalog.Arrangements.Any(a => a.Steps.Any(s => s.SceneId == id)))
            throw new InvalidOperationException("Remove this scene from its arrangements before deleting it.");
        var result = catalog with {
            Scenes = catalog.Scenes.Where(s => s.Id != id).ToImmutableArray(),
            Automation = catalog.Automation.Where(a => a.SceneId != id).ToImmutableArray()
        };
        result.Validate();
        return result;
    }
}
