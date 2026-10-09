# Workspaces and interaction design

Current delivery: [STATUS.md](STATUS.md). Build and test evidence: [VERIFICATION.md](VERIFICATION.md).

Perform follow-up (2026-10-07): eight chord pads include upper-octave I ↑. Key/scale buttons
are directly accessible beside the other performance controls. Position-independent pad
velocity prevents glissando accents; Ribbon retains vertical expression. SLOOP mode routes
by selected track and hides MIDI override controls; generic mode keeps manual channel choice.
Recording is collapsed below the instrument. Right-side ↕ rails scroll without playing,
and same-page refreshes retain scroll position.

Octave/latch follow-up (2026-10-08): Oct−/Oct+ update the register for the next chord press.
Latched chords keep their current sounding notes; other held notes revoice immediately.
Controls are not rebuilt and latch remains enabled. Automatic voice leading
keeps the root at its requested pitch, so I→vii→I cannot promote the tonic into the next
octave; I ↑ remains distinct. Explicit inversions remain intentional.

MIDI/octave checkpoint (2026-10-07): Perform now exposes Oct-/Oct+ and channel selection directly, with app base/effective octave and hardware offset in the readout. The connection menu offers generic MIDI alongside SLOOP, simulation, disconnect and details. Status details use a separate dialog so Android does not hide list choices; slot replacement/restore similarly separates review text from slot selection.

Status: all five workspaces have implemented controls; checkpoint 2026-10-07. The native
UI now uses a compact shared header, connection menu, teal selected track/workspace buttons,
rounded controls and expandable editing sections. The fuller product targets below do not
imply that scenes, piano roll, waveform zoom or global transport are implemented.

| Workspace | Available now | Remaining UI targets |
| --- | --- | --- |
| Perform | Chords/joystick, scale grid, chromatic keys, drum pads, ribbon; styles, latch/release, settings | Presets, recording, macros/FX/scenes, accessibility alternatives |
| Sequence | 64-position app editor and transformations; one-shot preview; native read/edit/send | Piano roll, detailed native step/lock inspector, scenes/arrangements |
| Sound | Six operators/global voice/macros, templates, SysEx, prompt diffs/local apply, FM6 RAM transfer | Algorithm/envelope graphs, operator copy/solo, all-engine/preset-bank UI |
| Sample | Capture/import, waveform/trim/chops, source/encoded preview, mapping/fit/export/slot transfer | Zoom, transient detection, marker drag, local polyphonic pads |
| Library | Export workspace/proposal/import/slot-backup files; restore slot images | Named sessions, full asset browser, search, portable archive import |

The disabled global transport placeholders have been removed. Connection, simulator and
disconnect actions share one menu, with full status available there. All five bottom tabs and
track selectors are reachable. Perform keeps latch/release above the playing surface and
puts help and destination checks in an expandable section. Status uses a fixed two-line
performance readout so held touches do not move
when text changes. Custom touch surfaces use pointer IDs and block scroll interception;
TalkBack virtual cells and equivalent joystick controls remain unimplemented.

Sound groups files/templates, prompts, device exchange, operators, globals and macros.
Operators have separate tone, envelope and keyboard-response sections; the selected operator
is highlighted. Sequence shows one selected bar as two rows of eight steps, with bar and step
selection highlighted; advanced transforms, note lists and hardware editing are expandable.
Library groups workspace files, slot recovery images and original imports.

Sample has **Chop** and **Send to FM1** pages. Record/import stays accessible, output selection
is explicit, and the Chop page shows a 180 dp waveform with tap/drag cursor and fine slider.
Split/Undo and Play/Stop are paired; trim/equal chopping is expandable and equal slice count
survives view rebuilds. Chops use a four-column audition pad bank with a highlighted selection and selected-chop export. The Send page shows mapping, slot
capacity, kit name/root, preparation, encoded audition and transfer. Backup/send/readback status
and upload percentage are visible, with success tied to verified readback. Editing is hidden
while a device operation is busy. A changed sample/mapping/name/root requires preparation
again before sending. Cursor dragging does not move existing chop boundaries; zoom, boundary
handles and transient detection remain planned.

## Persistent frame

Header: session name, connection/route status and clock source. Track strip: three synths plus
drums, selected track, mute/arm state. Transport: play/stop, record-mode selector, tempo and
position. Workspace navigation: Perform, Sequence, Sound, Sample, Library. Scene launcher lives
in Perform and arrangement/chain editing in Sequence; they share one scene model.

Portrait uses a focused page with a bottom detail sheet. Landscape/tablet adds a detail/mixer
panel. Preserve editing context on rotation. Audio services remain outside Activity lifecycle.
Respect system insets, use density-independent dimensions and scalable typography. The initial
programmatic shell is disposable layout scaffolding, not the final visual design.

## Workspace composition

| Workspace | Main surface | Secondary controls |
| --- | --- | --- |
| Perform | Pads/keys/chords selector, playable surface | Joystick/macros, scenes, mixer |
| Sequence | Drum grid or constrained piano roll | Step inspector, lanes, chain/arrangement |
| Sound | Engine-specific editor | Preset browser, envelopes, effects, FM6 operators |
| Sample | Waveform and slices | Capture/import, pads, mapping, capacity/upload sheet |
| Library | Sessions/assets/takes/presets/backups | Search, preview, import/export, recovery |

## Sample interaction

Capture input and meters stay visible. Waveform supports pinch zoom, horizontal scroll, marker
drag and a precise boundary inspector. Distinguish moving the playback cursor from moving a
slice marker. During marker drag show time/frame and audition the boundary when requested.
Chop method sheet previews generated markers before Apply. Slice selection highlights its pad
and mapping; tap plays, edit mode exposes boundary handles. Upload sheet shows destination,
existing contents, selected slice count, encoded size, stopped-state requirement and backup.

## Performance and editing gestures

Performance mode prioritizes immediate touches and prevents navigation/scroll from stealing
active pad gestures. Editing mode prioritizes selection and inspectors. Never use a hidden
long-press as the only route to a feature. Provide accessible alternatives for joystick/chord
quality, velocity entry, numeric parameters and precise waveform markers.
Step tap toggles; drag paints within a deliberate tool; selection mode prevents accidental
erase. Undo groups strokes. No app function depends on pressure-sensitive hardware.

## Status vocabulary

Offline: local editing available. Connected: MIDI handshake complete. Audio ready: validated
route available. Pending: intent sent. Applied: device acknowledged actual result. Queued:
future boundary accepted. Unsupported: firmware lacks capability. Interrupted: operation stopped
with recoverable artifacts. Uncertain: mutation may have applied and needs reconciliation.
Avoid generic green 'connected' indicators that imply every subsystem is ready.

Disable unavailable controls with a concise reason/action. Routine reads and reversible edits
do not need confirmation. Concrete replacement/restore sheets confirm destructive device data
changes; do not scatter modal approvals through the playing flow. Perform's app-owned
Release/stop stays reachable; a firmware-wide global panic is not implemented.
Recording source changes require ending the current take instead of silently swapping routes.

## Accessibility and verification

Use at least 48 dp interactive targets where practical, text and shapes alongside track colors,
descriptive accessibility labels, scalable text, and high contrast. Custom grids/waveforms need
accessible selection/detail controls. Check small portrait phones, landscape, tablet, large text,
screen reader navigation, system insets and simultaneous touch. Do not animate a fake meter or
playhead while offline. Capture screenshots at implementation milestones for visual review.


