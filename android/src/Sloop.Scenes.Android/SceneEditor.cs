using Android.Content;
using Android.Views;
using Android.Widget;

namespace Sloop.Scenes.Android;
/// <summary>Independent native view. Host owns state, transport-thread dispatch and persistence.</summary>
public sealed class SceneEditor : LinearLayout
{
    public SceneEditor(Context context, SceneCatalog catalog, string status,
        Action<Guid, SceneBoundary> queue, Action cancel, Action capture) : base(context)
    {
        Orientation = Orientation.Vertical;
        AddView(new TextView(context) { Text = "Scenes", TextSize = 18 });
        AddView(new TextView(context) { Text = status, TextSize = 14 });
        var boundaries = new Spinner(context);
        boundaries.Adapter = new ArrayAdapter<string>(context, global::Android.Resource.Layout.SimpleSpinnerDropDownItem,
            new[] { "Next beat", "Next bar", "Next phrase" });
        boundaries.SetSelection(1); AddView(boundaries);
        foreach (var scene in catalog.Scenes)
        {
            var button = new Button(context) { Text = $"{scene.Name} · {scene.Tempo} BPM", ContentDescription = $"Queue scene {scene.Name}" };
            button.Click += (_, _) => queue(scene.Id, (SceneBoundary)boundaries.SelectedItemPosition);
            AddView(button, new LayoutParams(ViewGroup.LayoutParams.MatchParent, ViewGroup.LayoutParams.WrapContent));
        }
        var actions = new LinearLayout(context) { Orientation = Orientation.Horizontal };
        void Button(string label, Action action)
        {
            var button = new Button(context) { Text = label }; button.Click += (_, _) => action();
            actions.AddView(button, new LayoutParams(0, ViewGroup.LayoutParams.WrapContent, 1));
        }
        Button("Capture scene", capture); Button("Cancel queued", cancel); AddView(actions);
        foreach (var chain in catalog.Arrangements)
            AddView(new TextView(context) { Text = $"{chain.Name} · {chain.Steps.Length} steps{(chain.Loop ? " · loop" : "")}", TextSize = 14 });
        for(int i=0;i<ChildCount;i++)if(GetChildAt(i) is TextView text)text.SetTextColor(global::Android.Graphics.Color.Rgb(230,233,240));
    }
}
