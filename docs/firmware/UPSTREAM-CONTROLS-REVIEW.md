# Remaining upstream controls and visual polish

Source review against upstream `ec9e537` (the firmware source is unchanged from
`b77bf55`), after MerthSloop commit `9fd9ddc`. This is an integration plan,
not a claim that the controls below are implemented. The validated production
package remains 2.7.1.49.

## Recommended order

1. Reconcile upstream device-setting bit packing and add round-trip fixtures.
2. Add STEP ADVANCE and DRUM PREVIEW preferences, plus noise-drum tuning.
3. Add adjustable track-filter resonance and punch fade, preserving our effects.
4. Add the playing-section badge to HOME; retain our existing themed SONG view.

The current target image uses all 584,336 application bytes. Recover and measure
space before packaging additions. No byte estimates are claimed from source
line counts. These changes need no additional audio delay buffers, but their
linked code, strings and settings state still have a cost.

## Effects controls

**Track-filter resonance:** upstream exposes a device-wide 0–10 preference,
mapping to SVF resonance 40–110. It affects synth and drum track filters, not
the master DJ filter or tape tone. We already have a resonance-capable helper
(`djf_block_res`) for wah, so reuse that helper instead of importing a second
filter implementation. Ordinary track filtering currently uses 40; wah uses
116 and must keep its own envelope/LFO behavior. The drum filter already has
a page and must receive the new resonance too.

Suggested placement: MENU → AUDIO. AUDIO already has three rows in the normal
non-CDC build, so resonance can occupy knob 4. The conditional CDC build has
an extra USB SERIAL row: do not silently create a fifth knob row there.
Test maximum-resonance hot input, swept cutoff, off/bypass, both CPU modes,
drum sends and entry/exit from wah. This adds coefficient control rather than
a new per-sample filter stage; actual target timing remains to be measured.

**Punch fade:** upstream uses FX + SELECT, 0–15, with a displayed value and a
gain increment `512 / (1 + 17 * value)`. Zero retains the current approximately
1.5 ms anti-click ramp; maximum is approximately 0.37 seconds. Calculate once
per block, not per sample.

Our punch path has independent primary, filter and gate gain ramps, all using
512 today. Apply a shared ramp setting to the three engagement envelopes;
keep the gate's internal rhythmic edges fast. Preserve the priority/fallback
of physically held and MIDI-held sources, FREEZE exceptions and effect switching.
HALF/DOUBLE are transport gestures and should remain immediate. LOFI is routed
through the master coloration path, outside these three ramps. It already
smooths wow/tone/crackle targets and wet gain at fixed rates; reconcile those
rates with the new control if it is to cover LOFI too. Do not claim a uniform
fade while leaving that route on its separate fixed smoothing.

## Drum and editor preferences

**DRUM PREVIEW:** audition is already implemented and currently unconditional
when browsing drum lanes/steps. Upstream adds the ability to disable browsing
audition (default OFF). Gate browsing calls consistently on GRID and SOUND;
keep deliberate note/pad playback and deliberate preview controls functional.
TUNE/DECAY adjustment currently auditions only while stopped; preserve that
distinction unless deliberately changing its behavior. Test that switching
preview OFF while a song runs does not inject extra hits or mute playback.

**STEP ADVANCE:** default ON preserves current behavior. OFF closes the current
key-entry transaction when physical and MIDI notes are released, leaving the
cursor in place for another attempt. It must still clear `entry_open`; merely
skipping `cursor_set` leaves the next chord in the previous entry transaction.
Add a SEQ settings section for these two preferences, rather than overloading
OCT gestures used by move/tie/rest/octave tools. Exercise physical and MIDI
release ordering, chords, existing independent ties and undo, with both settings.

**Noise-drum TUNE:** upstream shifts filter cutoff and optional high-pass
cutoff for sounds with no tonal oscillator. Our `ds_on_sound` currently changes
oscillator pitch but not that noise spectrum, so this is a real missing sonic
improvement. Apply the cutoff shift before filter coefficients are calculated;
use our shared factory/user sound path so it works for both kit sources. This
is note-on work, not extra per-sample DSP. Test positive/negative/extreme tune,
noise-only versus tonal drums, velocity, and user kits. Default tuning should
retain existing audio goldens; add targeted nonzero-tune renders.

Do not import upstream's black-key drum-lane/OCT paging remap as a dependency:
it conflicts with our existing navigation and held editing gestures. FILTER
access, piano-roll zoom and NOTE/HIT actions are already integrated.

## Settings compatibility comes first

Upstream reuses bytes in the canonical device-settings MIDI suffix:

| Byte after MIDI learn map | Existing low bits | New bits |
| --- | --- | --- |
| 3 | drum channel bits 0–4; OTHER bit 7 | preview bit 5; advance-off bit 6 |
| 4 | bend range bits 0–3 | punch fade bits 4–7 |
| 6 | drum map bit 0; clock out bit 1 | resonance bits 2–5 |

Our current BEND reader validates the entire byte as 1–12; a nonzero upstream
fade therefore causes a fallback bend range. Our DR MAP/CLOCK OUT reader
similarly rejects bytes above 3. The channel reader already masks bits 0–4
correctly, but does not restore the two new preferences. Correct both read and
write paths, defaults, change detection and deferred saves together. Preserve
our MRS1 side record and scale-light settings. These are device preferences,
not extra persisted FUN6 song globals. If adding upstream's runtime G_FXFADE,
audit global-array bounds, parameter descriptors and project serialization.

Fixtures should cover old/default settings, all new bits together, invalid
resonance values, restart, backup/restore and saving after an unrelated setting
change. Verify MIDI channel, bend range and clock output survive every case.

## Small visual details

**SONG colors are already integrated:** section tiles and bars use
`TE_COL[scene]` / `TE_DIM[scene]`, and the redraw signature includes theme colors.
There is no need to port upstream's SONG drawing code over our NEON layout.

**Playing-section badge is still useful:** upstream shows the active A–D section
on TRACKS. Adapt it to our HOME rather than copying the old header: show a small
colored section badge only while an arrangement section is actually running.
Use the running arrangement entry, not the selected saved slot or song-editor
cursor. Include arrangement running/index/scene in the redraw signature; our
current HOME body signature does not include them. Check same-track/same-preset
section changes, loop mode, stop/restart and all themes with screenshot tests.

Keep the existing preset-left/algorithm-right help, engine/kit name, header
spacing, held-action hints and footer clipping fixes. New controls need short,
accurate help and a visible value without covering the musical guide.

## Validation and scope

This review inspected source and diffs; it did not change firmware behavior,
run a new candidate target build, measure new CPU cost or flash a device.
Existing .49 test/build evidence remains described in the feature documents.
For implementation, run targeted settings/UI/audio tests, native/WASM parity,
target placement and budget checks, then publish a separately versioned package.
The upstream overlapping-note recording/bit-7 legato and preset-owned-arp policy
remain separate integration work.
