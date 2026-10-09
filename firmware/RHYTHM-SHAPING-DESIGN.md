# Native rhythm shaping

Shared pure mapping for ROM drum grooves and musical starters. Firmware owns the controls; an Android editor can expose the same choices later. The helper in `src/rhythm_shapes.c` has no persistent state or pattern buffers.

## Controls and order

Native drum SHAPE exposes target lane, SHIFT, SYNC and FEEL. Musical SEQUENCES has
a rhythm subpage with ROTATE, OFFSET, SYNCOPATE and FEEL, targeting its selected synth.
Both audition and apply rebuild from the same ROM definition. Phone drum apply remains
canonical; there is no new rhythm-shaping wire command in this release.

`rhythm_shape_t` contains signed step `rotate`, selected-part `offset`, signed `feel`, selected `part` (`255` means all), and `sync` (`0` off, `1` alternating beats, `2` every beat).

1. Syncopation exchanges an occupied beat start with the preceding empty step. An occupied destination is left alone, preserving all events and avoiding accidental overwrites. Alternating selects beat starts zero, two, four and so on.
2. Rotation moves the complete pattern by a whole number of steps, wrapping within the starter length.
3. Offset adds another step shift to one chosen lane or part, or to all parts. For example, shift hats one sixteenth later while leaving kick and snare alone.
4. Feel adds a signed nudge in the engine's existing units: 1/64 of a step, clamped to -32 through 31. Negative pushes early; positive lays back. This is independent of song swing.

Syncopation is a deterministic permutation, not random deletion or probabilistic playback. Moving entire event payloads retains velocity, drum articulation, notes and other metadata. Dense lanes remain unchanged by syncopation when preceding steps already contain events. Rotation and offset still affect those lanes.

## API

`rhythm_shape_source(shape, length, destination, part, original_occupied_mask, beat_steps)` returns the original source index to retrieve for a destination step. Call it separately for each drum lane or musical part, using that part's original occupied mask. Positive shift moves events later. It supports lengths 1 through 64, including partial and triplet bars. Invalid lengths return zero; absent configuration gives identity. Beat spacing zero or one disables syncopation. Partial-bar wrap pairs are skipped when their target would itself be a beat start, maintaining disjoint swaps.

`rhythm_shape_micro(shape, original_micro)` returns the clamped nudge. Drum microtiming is stored once per step in the existing format, so feel applies to the whole drum step; independent lane offsets are whole-step movements. No per-lane microtiming format extension is introduced.

Consumers rebuild preview and applied output from the ROM starter and current configuration on every change. They must not repeatedly transform the last preview or the user's current pattern: that would accumulate rotations, syncopation and feel. Preview remains separate from the saved pattern; Apply replaces the selected track through its normal undo and stopped-transport guard.

The consumer defines part selection labels and beat spacing from the starter division. Occupancy must include any event that cannot be overwritten. Starters containing tie chains need a consumer policy preserving those chains; this helper maps individual steps and does not reinterpret note durations.

## Resource and merge boundaries

No new persistent RAM or large stack buffers. Source mapping needs only scalar locals; masks are 64-bit values already present in ROM starters. Include guards permit the existing single-translation-unit firmware and standalone host tests to share this one file. Integration requires small include and call sites instead of rewriting the sequencer or its timing engine, keeping future upstream merges localized.

`tests/rhythm_shapes_test.c` checks zero configuration, lane isolation, signed wrap at step 64, syncopation into rests, occupied-target preservation, deterministic output, timing clamps, and one-to-one event mapping across all lengths 1–64 and beat spacings 0–5. Sanitizer execution catches invalid shifts and arithmetic errors in boundary cases.
