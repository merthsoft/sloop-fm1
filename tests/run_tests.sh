#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Copyright (C) 2026 Leo Kuroshita (@kurogedelic), Hügelton Instruments
# Host tests (no hardware). Run from the repo root after ./build.sh:
#   tests/run_tests.sh
#
# Regression suite (tests/regress.c, tests/target_budget.py; details at the top of regress.c):
#   golden renders  every engine x preset, the drum kit, voice modes, FX sends, a 4-track mix: one hash
#                   each in tests/golden.txt. A change of the sound fails with the list of renders.
#   health          clipping, DC, peak level, voices free after the release, silence at the end.
#   CPU             cost / sample per preset and mix, relative to the idle + drums mix (tests/cpu_baseline.txt,
#                   +25 %; counted by the kernel or under callgrind, else timed at +35 %), ns printed;
#                   target: loop instructions of the render functions in build/felucca.dis
#                   (tests/target_budget.txt, +10 %; exact, static).
#   voices          the budget of 8, steal fades, MONO / LEGATO / UNISON keep their note, the VOICE cap,
#                   no hanging notes on any MIDI / key routing.
# FM6 (tests/fm6_test.c): the 6-operator FM engine (firmware/src/eng_fm6.c, fm6_core.c, fm6_bank.c): the 32
#                   algorithms' carriers, the operator envelopes ending the voice, retrigger, DC / clipping, the
#                   eight macros, the patch formats (packed, SysEx), the 6-voice cap, the patch bank on a
#                   simulated NOR; demos in build/fm6_demo/.
# After an intended change of the sound: GOLDEN_UPDATE=1 sh tests/run_tests.sh, review the diff
# of tests/golden.txt, commit it with the change. After an intended change of the cost (or a new
# compiler): BUDGET_UPDATE=1 (rewrites cpu_baseline.txt and target_budget.txt). VERBOSE=1: every render.
set -e
export AC79_SDK="${AC79_SDK:-$HOME/fw-AC79_AIoT_SDK}"
cd "$(dirname "$0")/.."
OUT=build/host
mkdir -p "$OUT"
CC="${CC:-cc} -UNDEBUG -O1 -Wall -Wno-unused-function"
fail=0
run() { label="$1"; echo "== $label"; shift; "$@" || { status=$?; echo "FAILED ($status): $label"; fail=1; }; }

[ -f build/felucca.fwsc ] || { echo "run ./build.sh first"; exit 1; }
# the generated headers the FM6 engine needs (tools/build.py generate() makes them too; no Pillow needed)
mkdir -p build/gen
[ build/gen/felucca_tables.h -nt tools/gen_tables.py ] || python3 tools/gen_tables.py build/gen/felucca_tables.h
[ build/gen/felucca_fm6.h -nt tools/gen_fm6_patches.py ] || python3 tools/gen_fm6_patches.py build/gen/felucca_fm6.h

# Font packing uses the original BDF pixels as its oracle, then exercises the
# actual C renderer with both packed assets and the four-bit per-font format.
python3 tests/font_asset_test.py
$CC -Ibuild/host/font-assets -Ifirmware/src -o "$OUT/font_render_test" tests/font_render_test.c
run "packed font: identical pixels, metrics, scaling and clipping" "$OUT/font_render_test"
$CC -Ibuild/host/font-assets/legacy -Ibuild/host/font-assets -Ifirmware/src -o "$OUT/font_render_legacy_test" tests/font_render_test.c
run "four-bit font renderer compatibility" "$OUT/font_render_legacy_test"
python3 tests/icon_assets_test.py
$CC -O2 -w -Ibuild/host/icon-assets -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/icon_render_test" tests/icon_render_test.c -lm
run "lossless icon rows: source pixels, alpha and clipping" "$OUT/icon_render_test"
$CC -O2 -w -Ibuild/host/icon-assets/legacy -Ibuild/host/icon-assets -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/icon_render_legacy_test" tests/icon_render_test.c -lm
run "legacy icon renderer compatibility" "$OUT/icon_render_legacy_test"
$CC -o "$OUT/rhythm_shapes_test" tests/rhythm_shapes_test.c
run "rhythm shapes preserve hits, wrap rotation, syncopate and offset safely" "$OUT/rhythm_shapes_test"

$CC -o "$OUT/storage_test" tests/storage_test.c
run "flash storage (A/B, torn writes)" "$OUT/storage_test"

$CC -o "$OUT/recovery_test" tests/recovery_test.c
run "application USB recovery and boot-loop guard" "$OUT/recovery_test"

$CC -o "$OUT/arranger_test" tests/arranger_test.c
run "song order, timing, repeats and missing scenes" "$OUT/arranger_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/song_audio_test" tests/song_audio_test.c -lm
run "song: four simultaneous tracks, scene transition and stop" "$OUT/song_audio_test" "$OUT/song-demo.wav"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/song_ui_test" tests/song_ui_test.c -lm
run "song screen: commands, load (OCT+ twice), display bounds" "$OUT/song_ui_test" "$OUT/song-screen.ppm"

$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/studio_drums_test" tests/studio_drums_test.c -lm
run "drum lanes, kit audio, metronome, record arm, free take" "$OUT/studio_drums_test" "$OUT/drum-styles.wav"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/seq2_test" tests/seq2_test.c -lm
run "sequencer 2.0: no drift, ratchets, roll, erase / undo, ghost / hard, chords, mute / solo, nudge, locks" "$OUT/seq2_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/seq_note_ties_test" tests/seq_note_ties_test.c -lm
run "independent note ties: overlapping gates, voice identity, MIDI, ratchets, loops and save/load" "$OUT/seq_note_ties_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/seq_arp_test" tests/seq_arp_test.c -lm
run "sequencer-fed arpeggio: source ownership, microtiming, latch and generated MIDI cleanup" "$OUT/seq_arp_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/seq_arp_modes_test" tests/seq_arp_modes_test.c -lm
run "sequencer arpeggio: four-note chord plus fifteen ties, every runtime mode, generated MIDI grid" "$OUT/seq_arp_modes_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/seq_arp_expression_test" tests/seq_arp_expression_test.c -lm
run "arpeggio expression: source velocities, octave collisions, pulse, recording and preview takeover" "$OUT/seq_arp_expression_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/seq_navigation_test" tests/seq_navigation_test.c -lm
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/swing_grid_test" tests/swing_grid_test.c -lm
run "swing grid: HALF TIME audio timing, preview, legacy equivalence, settings and visualizer" "$OUT/swing_grid_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/note_ties_ui_test" tests/note_ties_ui_test.c -lm
run "native per-note tie selection, overlap, shortening, deletion and undo" "$OUT/note_ties_ui_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/scale_lights_test" tests/scale_lights_test.c -lm
run "persistent scale lights: physical gesture, pitch classes and settings" "$OUT/scale_lights_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/modulation_test" tests/modulation_test.c -lm
run "live modulation wheel: priority, restoration, bounds and cleanup" "$OUT/modulation_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/lofi_chord_arp_test" tests/lofi_chord_arp_test.c -lm
run "LOFI chip arp: legacy and complete SEL chord shapes" "$OUT/lofi_chord_arp_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/keyboard_handoff_test" tests/keyboard_handoff_test.c -lm
run "keyboard octave handoff: shared hold without reattack" "$OUT/keyboard_handoff_test"

$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/step_move_merge_test" tests/step_move_merge_test.c -lm
run "STEP move brush: merge a mistimed note into a chord" "$OUT/step_move_merge_test"
run "native step painting/moves, record snap controls, SEQ navigation and SNOTE arp modes" "$OUT/seq_navigation_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/edit_tools_test" tests/edit_tools_test.c -lm
run "EDIT tools: scale guides, whole-pattern octaves, gesture readouts and undo" "$OUT/edit_tools_test"

$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/drumkit_test" tests/drumkit_test.c -lm
run "synthesised drum kits: every kit x sound bounded, audible, finite, levels, cost" "$OUT/drumkit_test" "$OUT/drum-kits.wav" "$OUT/drum-kits.txt"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/userkit_test" tests/userkit_test.c -lm
run "user drum kits (KIT USR1..USR3): a user slot's sounds on the drum lanes" "$OUT/userkit_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/dsyn_test" tests/dsyn_test.c -lm
run "SYN1..SYN4 drum kits and the editor's DRUM SYNTH commands (v10)" "$OUT/dsyn_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/font_test" tests/font_test.c
run "the screen font at 1 bit a pixel: every glyph, the same pixels as 2.5" "$OUT/font_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/effects_plan_test" tests/effects_plan_test.c -lm
run "HALF transport, effect combinations and optional SPRING" "$OUT/effects_plan_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/spring_time_test" tests/spring_time_test.c -lm
run "SPRING independent reflection timing and FREEZE" "$OUT/spring_time_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/reverb_types_test" tests/reverb_types_test.c -lm
run "reverb types: measured tails, stereo, damping, overload and FREEZE lifecycle" "$OUT/reverb_types_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/vinyl_test" tests/vinyl_test.c -lm
run "persistent VINYL DSP and bypass" "$OUT/vinyl_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/lofi_character_test" tests/lofi_character_test.c -lm
run "LOFI master: character changes, stereo, overload, silence and captured fragments" "$OUT/lofi_character_test"
$CC -UNDEBUG -O2 -Wall -Wextra -o "$OUT/wet_color_test" tests/wet_color_test.c -lm
run "wet coloration foundation: exact bypass, stereo, transitions, tape and crush" "$OUT/wet_color_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/preset_browser_map_test" tests/preset_browser_map_test.c -lm
run "preset ROM indices: canonical names, browser positions and favourite keys" "$OUT/preset_browser_map_test"
$CC -DMIDI_REQUIRE_DRUM_HOOK=1 -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/midi_expression_bank_test" tests/midi_expression_bank_test.c -lm
run "MIDI expression: synth/drum audio, bank mapping and load reset" "$OUT/midi_expression_bank_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/midi_bank_ui_test" tests/midi_bank_ui_test.c -lm
run "MIDI Program Change: production factory/user loading and preference preservation" "$OUT/midi_bank_ui_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/effects_cpu_test" tests/effects_cpu_test.c -lm
run "effects host profile and two-core identity" "$OUT/effects_cpu_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/punch_test" tests/punch_test.c -lm
run "punch-in FX: 27 effects (11 on the black keys), bounded, dry after release, FX-held keys" "$OUT/punch_test" "$OUT/punch-fx.wav"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/fx251_test" tests/fx251_test.c -lm
run "after Felucca 1.4 / 1.5: INSERT, filter TYPE, ENV SYNC, LFO 2, drum mutes, MIDI LEARN" "$OUT/fx251_test"

$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/performance_remote_test" tests/performance_remote_test.c
run "remote performance: wire validation, leases and owner cleanup" "$OUT/performance_remote_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/performance_engine_test" tests/performance_engine_test.c -lm
run "remote performance: actual panel priority, DSP and sequencer boundaries" "$OUT/performance_engine_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/drum_grooves_test" tests/drum_grooves_test.c -lm
run "drum groove starters: apply, undo, project roundtrip and screens" "$OUT/drum_grooves_test" "$OUT"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/sequence_starters_test" tests/sequence_starters_test.c -lm
run "24 musical starters: voice leading, roots, scales, rhythm transforms and preview" "$OUT/sequence_starters_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/sequence_starters_ui_test" tests/sequence_starters_ui_test.c -lm
run "musical browser: all scales, pitch controls, audition, hold apply and complete undo" "$OUT/sequence_starters_ui_test" "$OUT"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/note_trails_test" tests/note_trails_test.c -lm
run "note trails: transient capture, full pitch range, overlap colors and mute" "$OUT/note_trails_test" "$OUT"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/track_fx_ui_test" tests/track_fx_ui_test.c -lm
run "track FX: preserve across instruments, explicit reset and track isolation" "$OUT/track_fx_ui_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/editor_drum_grooves_test" tests/editor_drum_grooves_test.c -lm
run "phone drum grooves: capabilities, malformed requests, confirmation and undo" "$OUT/editor_drum_grooves_test"
$CC -O2 -Wall -Wextra -o "$OUT/harmony_owners_test" tests/harmony_owners_test.c firmware/src/harmony_owners.c
run "inactive harmony foundation: shared-owner note transitions" "$OUT/harmony_owners_test"

$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/ui_pages_test" tests/ui_pages_test.c -lm
run "live UI: pages, layers (punch, steps, erase, roll, key, mix), holds, drums, REC, fuzz" "$OUT/ui_pages_test" "$OUT"
$CC -UNDEBUG -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/piano_roll_test" tests/piano_roll_test.c -lm
run "piano roll: zoom, clipped sustains, held tools, redraw parity and immutable music" "$OUT/piano_roll_test" "$OUT"
$CC -UNDEBUG -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/piano_note_selection_test" tests/piano_note_selection_test.c -lm
run "piano selection foundation: stable attacks, reverse navigation and independent ties" "$OUT/piano_note_selection_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/piano_notes_review_test" tests/piano_notes_review_test.c -lm
run "NOTES review: undo, redraw, held tools, epoch invalidation and encoder bounds" "$OUT/piano_notes_review_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/piano_actions_ui_test" tests/piano_actions_ui_test.c -lm
run "note/hit sheets: staged edits, ownership, undo, exits and renderer parity" "$OUT/piano_actions_ui_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/note_chain_actions_test" tests/note_chain_actions_test.c -lm
run "chain actions: atomic pitch/move/copy, overlap, metadata and balanced MIDI" "$OUT/note_chain_actions_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/drum_hit_actions_test" tests/drum_hit_actions_test.c -lm
run "hit actions: lane isolation, dynamics, metadata and atomic refusals" "$OUT/drum_hit_actions_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/drum_fx_ui_test" tests/drum_fx_ui_test.c -lm
run "drum FX: real sends, level/pan, filter and populated insert blend page" "$OUT/drum_fx_ui_test" "$OUT"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/ui_refresh_test" tests/ui_refresh_test.c -lm
run "NEON UI: incremental/full redraw, palettes, drum cells, popup state, help" "$OUT/ui_refresh_test" "$OUT"
# no divide by 0 (the FM-1 runs with the div0 trap off, hal/fm1_irq.h: a real one would give a wrong value
# silently): the UI fuzz, the sequencer, the projects and a minute of random live use, with UBSan
UBSAN="${CC_UB:-cc} -UNDEBUG -O1 -w -fsanitize=integer-divide-by-zero -fno-sanitize-recover=integer-divide-by-zero -Ibuild/gen -Ifirmware/src -Ifirmware/hal"
if $UBSAN -o "$OUT/ui_pages_ub" tests/ui_pages_test.c -lm && $UBSAN -o "$OUT/seq2_ub" tests/seq2_test.c -lm &&
    $UBSAN -o "$OUT/seq_note_ties_ub" tests/seq_note_ties_test.c -lm &&
    $UBSAN -o "$OUT/project_ub" tests/project_test.c -lm && $UBSAN -o "$OUT/soak_ub" tests/soak_test.c -lm; then
    mkdir -p "$OUT/ub"
    run "no divide by zero (UBSan): UI fuzz, sequencer, projects, a minute of live use" \
        sh -c "'$OUT/ui_pages_ub' '$OUT/ub' >/dev/null && '$OUT/seq2_ub' >/dev/null && '$OUT/seq_note_ties_ub' >/dev/null && '$OUT/project_ub' >/dev/null && '$OUT/soak_ub' 1 >/dev/null && echo 'no divide by zero'"
else
    echo "UBSan build failed; not running stale test binaries"
    fail=1
fi

# the second core's host double (tests/cpu1_host.c, firmware/src/cpu1.h): inline, a thread, a slow thread, a lost
# job; and the thread under ThreadSanitizer (the render's shared state shows up here once it uses core 1)
$CC -w -Ifirmware/src -o "$OUT/cpu1_test" tests/cpu1_test.c
run "second core, host double: inline, deferred to the wait (CORES=1)" env CORES=1 "$OUT/cpu1_test"
run "second core, host double: a thread (CORES=2)" env CORES=2 "$OUT/cpu1_test"
run "second core, host double: a thread with random delays (CORES=2 CHAOS=7)" env CORES=2 CHAOS=7 "$OUT/cpu1_test"
run "second core, host double: a lost job, the hold and the fallback (CPU1_TIMEOUT_AT=777)" env CORES=2 CPU1_TIMEOUT_AT=777 "$OUT/cpu1_test"
TSAN="${CC_UB:-cc} -O1 -g -w -fsanitize=thread -Ifirmware/src"
if $TSAN -o "$OUT/cpu1_tsan" tests/cpu1_test.c 2>/dev/null; then
    run "second core, host double under ThreadSanitizer (CORES=2 CHAOS=3)" env CORES=2 CHAOS=3 "$OUT/cpu1_tsan"
else
    echo "(cpu1 under TSan: this compiler has no ThreadSanitizer, skipped)"
fi

$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/soak_test" tests/soak_test.c -lm
run "soak: ${SOAK_MIN:-10} minutes of random live use (bounded, no hanging voices, idle after stop)" "$OUT/soak_test" "${SOAK_MIN:-10}"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/stress_test" tests/stress_test.c -lm
run "stress (2.4): FONT_L = FONT_S at 2x, hard random use of every 2.4 addition, clean stop" "$OUT/stress_test" "${STRESS_FRAMES:-40000}"
# the same under AddressSanitizer + UBSan: any read or write out of bounds stops it (left shifts of negative values
# and the FM6 phase's wrap-around are left out: the DSP's two's complement idioms, as every compiler builds them)
ASAN="${CC_UB:-cc} -O1 -g -w -fsanitize=address,undefined -fno-sanitize=shift-base,signed-integer-overflow -fno-sanitize-recover=all -Ibuild/gen -Ifirmware/src -Ifirmware/hal"
if $ASAN -o "$OUT/stress_asan" tests/stress_test.c -lm 2>/dev/null; then
    run "stress under ASan + UBSan (no access out of bounds)" "$OUT/stress_asan" "${STRESS_ASAN_FRAMES:-15000}" 7
else
    echo "(stress under ASan: this compiler has no AddressSanitizer, skipped)"
fi

$CC -o "$OUT/upreset_test" tests/upreset_test.c
run "user presets (UP_PUT parser, bank round trip, versions)" "$OUT/upreset_test"

$CC -o "$OUT/midi_uart_test" tests/midi_uart_test.c
run "TRS MIDI parser" "$OUT/midi_uart_test"
$CC -Ifirmware/hal -o "$OUT/encoder_test" tests/encoder_test.c
run "knobs: one click = one step (slow, fast, pauses, bounce)" "$OUT/encoder_test"
HALF=$(sed -n 's/^#define HALF_FRAMES \([0-9]*\).*/\1/p' firmware/src/core.h)
$CC -DT_CDC=1 -DHALF_FRAMES=$HALF -o "$OUT/uac_test" tests/uac_test.c -lm
run "USB audio input: descriptors (with CDC), ring and packets" "$OUT/uac_test"
$CC -DT_CDC=0 -DHALF_FRAMES=$HALF -o "$OUT/uac_test_nocdc" tests/uac_test.c -lm
run "USB audio input: descriptors (without CDC), ring and packets" "$OUT/uac_test_nocdc"
$CC -DT_CDC=2 -DHALF_FRAMES=$HALF -o "$OUT/uac_test_seroff" tests/uac_test.c -lm
run "USB audio input: descriptors (CDC built in, menu USB SERIAL OFF), ring and packets" "$OUT/uac_test_seroff"
run "USB SERIAL OFF: the descriptors of a build without CDC, byte for byte" \
    sh -c "[ \"\$(UAC_DUMP=1 '$OUT/uac_test_seroff' | tail -n 2)\" = \"\$(UAC_DUMP=1 '$OUT/uac_test_nocdc' | tail -n 2)\" ] && echo same"
$CC -DT_CDC=2 -DT_RATE=1 -DHALF_FRAMES=$HALF -o "$OUT/uac_test_441" tests/uac_test.c -lm
run "USB audio input, menu USB RATE 44.1 (2.7): descriptors, ring and packets" "$OUT/uac_test_441"
$CC -DT_CDC=1 -DT_RATE=2 -DHALF_FRAMES=$HALF -o "$OUT/uac_test_48" tests/uac_test.c -lm
run "USB audio input, menu USB RATE 48 (2.7): descriptors, ring and packets" "$OUT/uac_test_48"
$CC -w -DT_CDC=0 -DFELUCCA_UAC_48K=0 -DHALF_FRAMES=$HALF -o "$OUT/uac_test_no48" tests/uac_test.c -lm
run "USB RATE 44.1: the descriptors of a build without 48 kHz (SLOOP 2.4), byte for byte" \
    sh -c "[ \"\$(UAC_DUMP=1 '$OUT/uac_test_441' | tail -n 2)\" = \"\$(UAC_DUMP=1 '$OUT/uac_test_no48' | tail -n 2)\" ] && echo same"
uac_in_app() { ${CC%% *} -E -Ibuild/gen -Ifirmware/hal -Ifirmware/src firmware/src/felucca.c 2>/dev/null | grep -q uac_service; }
run "USB audio input: built into the firmware (FELUCCA_UAC set before usb.c)" uac_in_app
run "scene transaction staging, faults and OTA USB compatibility" env CC="$CC" sh tests/run_scene_transaction.sh

$CC -o "$OUT/ota_test" tests/ota_test.c
run "M-UPGRADE entry" "$OUT/ota_test" build/felucca.fwsc
run "package capacity and SDK boundary guard" python3 tests/package_capacity_test.py

head -c 200000 build/felucca.bin > "$OUT/old_app.bin"
python3 tools/fm1pkg_make.py "$OUT/old_app.bin" build/loader/ota.bin "$OUT/old.fwsc" >/dev/null
$CC -o "$OUT/ldr_test" tests/ldr_test.c
run "update loader: other app -> this build" "$OUT/ldr_test" "$OUT/old.fwsc" build/felucca.fwsc

$CC -UNDEBUG -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal tests/delay_storage_test.c -o "$OUT/delay_storage_test" -lm
run "delay characters: project/section storage, old defaults and marker coexistence" "$OUT/delay_storage_test"
$CC -UNDEBUG -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal tests/delay_ui_test.c -o "$OUT/delay_ui_test" -lm
run "delay characters: native page and controls; LOFI OFF default" "$OUT/delay_ui_test" "$OUT"
$CC -UNDEBUG -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal tests/ui_shared_paths_test.c -o "$OUT/ui_shared_paths_test" -lm
run "shared UI paths: HOME isolation and physical/MIDI chord entry" "$OUT/ui_shared_paths_test"
$CC -UNDEBUG -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal tests/delay_character_test.c -o "$OUT/delay_character_test" -lm
for mode in 0 1 2; do
    run "delay character $mode: exhaustive tap bounds and live feedback/time/mode stress" "$OUT/delay_character_test" "$mode" "$OUT/delay-$mode.wav"
done

$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/hostsim" tests/hostsim.c -lm
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/scale_test" tests/scale_test.c -lm
run "scales: white-key mapping and note lifecycle" "$OUT/scale_test"
run "DSP render (ANALOG preset 0)" "$OUT/hostsim" 0 0 1 "$OUT/render.wav"
mkdir -p build/tracks_demo
run "TRACKS: 4-track pattern, live recording (lengths, swing), voice budget, engine switch, cost" env TRACKS=build/tracks_demo "$OUT/hostsim" 0 0 1 "$OUT/tracks.wav"
$CC -w -Ibuild/gen -Ifirmware/src -o "$OUT/project_test" tests/project_test.c -lm
run "project formats (FUN4 / FUN3 / FUN2 / FUN1 -> FUN5), capture / apply, autosave" "$OUT/project_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/section_save_test" tests/section_save_test.c -lm
run "section saves: stopped flash durability, deferred live queue and retries" "$OUT/section_save_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/upstream_project_test" tests/upstream_project_test.c -lm
run "upstream FUN6 compatibility: independent upstream-produced file roundtrip" "$OUT/upstream_project_test"
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/slicer_test" tests/slicer_test.c -lm
mkdir -p build/slicer_demo
run "SLICER: no clicks, timing, sync with the sequencer, STUT, cost, demos" "$OUT/slicer_test" build/slicer_demo
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/fm6_test" tests/fm6_test.c -lm
mkdir -p build/fm6_demo
run "FM6: algorithms, envelopes, retrigger, DC, clipping, macros, patch formats, voices, the bank, demos" "$OUT/fm6_test" build/fm6_demo
$CC -O2 -w -Ibuild/gen -Ifirmware/src -o "$OUT/regress" tests/regress.c -lm
# the CPU budget: counted by the kernel on macOS; elsewhere under callgrind when valgrind is there (exact, ~45 s;
# SKIP_CPU_VALGRIND=1 to time instead, which is only a rough check)
if [ -z "$SKIP_CPU_VALGRIND" ] && command -v valgrind >/dev/null 2>&1; then export CPU_VALGRIND=1; fi
run "regression: golden renders, health, voices, CPU budget" "$OUT/regress" tests/golden.txt tests/cpu_baseline.txt
# SLICE (tests/slice_test.c) needs a FELUCCA_SLICE=1 build; the engine is not built by default

run "regression: target cost of the render loops" python3 tests/target_budget.py \
    build/felucca.dis tests/target_budget.txt
# the render split over the two cores (fx.c mix_units): the golden renders and tests/mix_split_test.c (three
# FORMANT, FM6 or PHYS parts, S&H LFOs, mutes, the drums through the SLICER and the FILTER, the metronome: every
# split of the units against none) under every mode of the host double of core 1, and under ThreadSanitizer
run "the render split over the two cores: every mode of the second core's host double (tests/cpu1_modes.sh)" sh tests/cpu1_modes.sh

$CC -O1 -w -Ibuild/gen -Ifirmware/src -o "$OUT/boot_tables_test" tests/boot_tables_test.c
run "startup tables: original bytes and decoder bounds" "$OUT/boot_tables_test" build/gen/boot_tables_oracle.bin
$CC -UNDEBUG -O2 -w -Ibuild/gen -Ifirmware/src tests/boot_presets_test.c -o "$OUT/presets_raw" -lm
run "factory preset oracle: original C definitions" "$OUT/presets_raw" "$OUT/presets-raw.bin"
$CC -UNDEBUG -O2 -w -DFELUCCA_BOOT_TABLES=1 -DFELUCCA_BOOT_PRESETS=1 -Ibuild/gen -Ifirmware/src tests/boot_presets_test.c -o "$OUT/presets_boot" -lm
run "factory presets: actual startup restore and scratch bounds" "$OUT/presets_boot" "$OUT/presets-boot.bin"
run "factory presets: original and restored names/fields byte-identical" cmp "$OUT/presets-raw.bin" "$OUT/presets-boot.bin"


run "target DSP: preserve per-sample helper inlining" python3 tests/target_inlining_test.py build/felucca.dis

run "installer CLI (fm1_install.py) against a simulated FM-1" python3 tests/install_test.py
run "the offline editor (one file, nothing next to it); the offline site (sw.js, manifest)" python3 tests/site_test.py

if command -v node >/dev/null 2>&1; then
    run "web pages: editor protocol, samples, packages, update protocol" node web/test_web.mjs
    rm -rf "$OUT/pwa-site" && python3 web/make_site.py build/felucca.fwsc 2.7 "$OUT/pwa-site" >/dev/null &&
        run "the site without internet in Chromium (skipped without Playwright)" node tests/pwa_test.mjs "$OUT/pwa-site"
else
    echo "== skip web tests (no node)"
fi

# the browser audio preview (web editor): the same engines in WASM as on the host, when it is built
# (python3 tools/build_preview.py: Emscripten, or clang + wasm-ld)
if [ -f build/preview/sloop-preview.wasm ]; then
    run "the editor's audio preview: WASM == host, all 12 engines" sh tests/run_preview_tests.sh
fi
[ $fail -eq 0 ] && echo "ALL HOST TESTS PASSED" || { echo "HOST TESTS FAILED"; exit 1; }
