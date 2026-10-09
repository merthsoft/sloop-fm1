using Android.Graphics;
using Android.Graphics.Drawables;
using Android.Content.Res;
using Android.Views;
using Android.Widget;
using Orientation = Android.Widget.Orientation;

namespace Sloop.Android;

public sealed partial class MainActivity
{
    private static readonly Color Accent = Color.Rgb(82, 220, 188);
    private static readonly Color Panel = Color.Rgb(33, 40, 49);
    private readonly List<Button> workspaceTabs = [];
    private readonly Dictionary<string, bool> openSections = new();

    private void StyleButton(Button button, bool selected = false)
    {
        button.SetAllCaps(false);
        button.TextSize = 13;
        button.SetMinWidth(0);
        button.SetMinimumHeight(Dp(48));
        button.SetPadding(Dp(8), Dp(4), Dp(8), Dp(4));
        button.SetTextColor(new ColorStateList(
            [new[] { -global::Android.Resource.Attribute.StateEnabled }, Array.Empty<int>()],
            [Color.Rgb(108, 119, 130).ToArgb(), (selected ? Color.Rgb(15, 35, 31) : Color.Rgb(230, 236, 240)).ToArgb()]));
        var shape = new GradientDrawable();
        shape.SetColor(selected ? Accent : Panel);
        shape.SetCornerRadius(Dp(10));
        button.Background = new RippleDrawable(ColorStateList.ValueOf(Color.Argb(55, 255, 255, 255)), shape, null);
        if (button.LayoutParameters is LinearLayout.LayoutParams lp)
        {
            lp.SetMargins(Dp(2), Dp(3), Dp(2), Dp(3));
            button.LayoutParameters = lp;
        }
    }

    private void StyleContent(View view)
    {
        if (view is Button button && view is not CompoundButton) StyleButton(button, button.Activated);
        if (view is EditText input)
        {
            input.SetTextColor(Color.Rgb(235, 240, 244));
            input.SetHintTextColor(Color.Rgb(155, 169, 181));
            input.BackgroundTintList = ColorStateList.ValueOf(Accent);
            input.TextSize = 16;
            input.SetMinimumHeight(Dp(48));
        }
        if (view is SeekBar slider) slider.ProgressTintList = slider.ThumbTintList = ColorStateList.ValueOf(Accent);
        if (view is ViewGroup group)
            for (int i = 0; i < group.ChildCount; i++) StyleContent(group.GetChildAt(i)!);
    }

    private void Section(string key, string title, Action build, bool initiallyOpen = false)
    {
        bool expanded = openSections.GetValueOrDefault(key, initiallyOpen);
        var header = new Button(this) { Text = (expanded ? "▾  " : "▸  ") + title };
        content.AddView(header);
        var outer = content;
        var body = new LinearLayout(this) { Orientation = Orientation.Vertical, Visibility = expanded ? ViewStates.Visible : ViewStates.Gone };
        body.SetPadding(Dp(6), 0, Dp(6), Dp(8));
        outer.AddView(body);
        content = body;
        try { build(); } finally { content = outer; }
        header.Click += (_, _) =>
        {
            expanded = !expanded; openSections[key] = expanded;
            body.Visibility = expanded ? ViewStates.Visible : ViewStates.Gone;
            header.Text = (expanded ? "▾  " : "▸  ") + title;
        };
    }

    private void ButtonRow(params (string Title, Action Action, bool Enabled)[] actions)
    {
        var row = new LinearLayout(this) { Orientation = Orientation.Horizontal };
        foreach (var action in actions)
        {
            var b = new Button(this) { Text = action.Title, Enabled = action.Enabled };
            b.Click += (_, _) => action.Action();
            row.AddView(b, new LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WrapContent, 1));
        }
        content.AddView(row);
    }
}
