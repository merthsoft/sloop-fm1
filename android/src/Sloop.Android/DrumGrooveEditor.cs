using Android.Widget;
using Sloop.Protocol;

namespace Sloop.Android;
public sealed partial class MainActivity
{
    void AddDrumGrooveEditor()
    {
        content.AddView(Label("Apply a built-in FM1 groove to its drum pattern. The phone's app pattern is separate. Kit and tempo stay selected; hardware Undo restores the replaced pattern.", 14));
        bool ready = connection.Snapshot.Device?.ProtocolVersion >= 12 && !connection.Snapshot.IsSimulated &&
            !connection.Snapshot.IsGenericMidi && !connection.Snapshot.Busy && !connection.IsPlaying && !connection.IsRecording;
        AsyncButton("Browse FM1 groove bank…", async () => {
            var identity = connection.Snapshot.Device;
            var grooves = await connection.EditDeviceAsync("Reading FM1 groove bank…", (c, t) => c.ListDrumGroovesAsync(identity!, t));
            new global::Android.App.AlertDialog.Builder(this)!.SetTitle("FM1 drum groove starters")!
                .SetItems(grooves.Select(g => $"{g.Name} · {g.Length} steps").ToArray(), (_, a) => {
                    var groove = grooves[a.Which];
                    new global::Android.App.AlertDialog.Builder(this)!.SetTitle($"Apply {groove.Name}?")!
                        .SetMessage("Replace the FM1 drum pattern, timing, conditions and locks? Stop the FM1 first. Synth patterns, kit and tempo remain selected. Use EDIT + OCT− on FM1 to undo.")!
                        .SetNegativeButton("Cancel", (_, _) => { })!
                        .SetPositiveButton("Replace drum pattern", async (_, _) => {
                            try {
                                if (!ReferenceEquals(connection.Snapshot.Device, identity)) throw new IOException("Connection changed. Browse the groove bank again.");
                                var status = await connection.EditDeviceAsync("Applying drum groove…", (c, t) => c.ApplyDrumGrooveAsync(groove.Id, true, t));
                                if (status != DrumGrooveStatus.Applied) throw new IOException(status == DrumGrooveStatus.Busy ? "Stop FM1 playback and recording before applying a groove." : "Groove refused; browse the bank again.");
                                hardwareBaseline = null; // previously read native material is no longer a valid write baseline
                                editing.SetStatus($"{groove.Name} applied to FM1 drums. Read native hardware patterns to inspect/edit it; hardware Undo can restore the previous groove.");
                            } catch (Exception e) { editing.SetStatus(e.Message + " Read hardware before retrying if the outcome is uncertain."); }
                        })!.Show();
                })!.SetNegativeButton("Close", (_, _) => { })!.Show();
        }, ready);
        if (!ready) content.AddView(Label("Connect SLOOP with groove-bank support and stop app playback to browse.", 12));
    }
}
