# Focused piano-roll checks

Run `dotnet run --project android/src/Sloop.PianoRoll.Tests/Sloop.PianoRoll.Tests.csproj`
from the repository, or run `RunPianoRollChecks.ps1` here.

This package-free console runner links the exact platform-free `PianoRollModel.cs` used
by Android. It checks release-only creation, drag/cancel safety, finger-sized hit resolution,
selection, quantized move/resize, bounds, channel isolation, event/part locks, deletion,
one-gesture history, redo, stale proposals, viewport bounds, zoom and custom phrase lengths.
The 87 checks include toggle/additive/reverse-box/channel selection, group previews and
bounds, atomic move/resize/delete/quantize undo, short imported note durations, protection
added during a drag, late-member protection rejection, nonfinite releases and legal app
overlaps. Channel or source adoption cancels a captured gesture before its old view releases.
It does not exercise Android drawing, event dispatch or hardware timing.
