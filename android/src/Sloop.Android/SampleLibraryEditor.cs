using Android.Widget;
using Sloop.Workstation;

namespace Sloop.Android;

public sealed partial class MainActivity
{
    private async void BrowseCrossSessionSamples()
    {
        var cancellation = new CancellationTokenSource();
        var layout = new LinearLayout(this) { Orientation = Orientation.Vertical };
        var search = new EditText(this) { Hint = "Search sample or source session" };
        var status = new TextView(this) { Text = "Indexing saved sessions…" };
        var list = new ListView(this);
        var active = new Button(this) { Text = "Browse active session retained samples" };
        active.Click += async (_, _) => {
            try {
                var assets = await samples.RetainedAssetsAsync();
                if (IsFinishing || IsDestroyed || cancellation.IsCancellationRequested) return;
                new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Active session")!
                    .SetItems(assets.Select(a => $"{(a.Current ? "Current · " : "")}{a.Name} · {a.Seconds:0.00}s · {a.Chops} chops").ToArray(),
                        (_, args) => _ = samples.ReuseAssetAsync(assets[args.Which].Name))!
                    .SetNegativeButton("Close", (_, _) => { })!.Show();
            } catch (Exception error) { status.Text = error.Message; }
        };
        layout.AddView(active); layout.AddView(search); layout.AddView(status); layout.AddView(list, new LinearLayout.LayoutParams(-1, 650));
        var dialog = new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Sample library")!.SetView(layout)!
            .SetNegativeButton("Close", (_, _) => cancellation.Cancel())!.Create()!;
        dialog.DismissEvent += (_, _) => cancellation.Cancel();
        LibrarySample[] all = [], visible = [];
        void Filter() {
            var text = search.Text?.Trim() ?? "";
            visible = all.Where(a => a.Name.Contains(text, StringComparison.OrdinalIgnoreCase) || a.SessionName.Contains(text, StringComparison.OrdinalIgnoreCase)).ToArray();
            list.Adapter = new ArrayAdapter<string>(this, global::Android.Resource.Layout.SimpleListItem1,
                visible.Select(a => $"{a.Name}\n{a.SessionName} · {a.Seconds:0.00}s · {a.Chops} chops").ToArray());
        }
        search.TextChanged += (_, _) => Filter();
        list.ItemClick += async (_, args) => {
            if (args.Position < 0 || args.Position >= visible.Length) return;
            var selected = visible[args.Position];
            if (samples.Busy || samples.RecordingActive) return;
            // Closing the browser cancels indexing; reuse gets its own lifetime.
            dialog.Dismiss();
            using var copyCancellation = new CancellationTokenSource();
            var copying = new global::Android.App.AlertDialog.Builder(this)!.SetTitle("Reuse sample")!
                .SetMessage($"Copying {selected.Name} from {selected.SessionName} into the active session…")!
                .SetNegativeButton("Cancel", (_, _) => copyCancellation.Cancel())!.Create()!;
            copying.SetCanceledOnTouchOutside(false);
            copying.DismissEvent += (_, _) => copyCancellation.Cancel();
            copying.Show();
            try { await samples.ReuseLibrarySampleAsync(selected, copyCancellation.Token); }
            finally { copying.Dismiss(); }
        };
        dialog.Show();
        try {
            var index = await samples.CrossSessionSamplesAsync(cancellation.Token);
            if (cancellation.IsCancellationRequested || IsFinishing || IsDestroyed) return;
            all = index.Samples; Filter();
            status.Text = $"{all.Length} saved samples. Reuse copies into the active session." +
                (index.Rejected > 0 ? $" {index.Rejected} invalid assets/sessions skipped." : "") +
                (index.Truncated ? " Scan limit reached; some sessions were not indexed." : "");
        } catch (OperationCanceledException) { }
        catch (Exception error) { if (!cancellation.IsCancellationRequested) status.Text = "Library unavailable: " + error.Message; }
    }
}
