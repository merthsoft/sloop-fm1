# Live modulation wheel — Merthsoft.10

Physical LFO + knob 1 controls a runtime 0–127 modulation amount, four units per
detent, while held. Its normal page RATE edit is consumed for that knob only.
Releasing clears physical ownership; the latest incoming CC1 value becomes active
again. Track changes, menu/confirmation/control layers, panic and STOP cancel the
gesture. After STOP/panic, knob 1 is consumed until LFO is lifted so it cannot edit
RATE accidentally. LFO taps and other page controls remain intact. No note retrigger occurs.

MIDI CC1 follows existing channel routing: 1–3 synths, drum channel ignored,
other channels the selected track. IN CLOCK ignores it. Values are per synth,
last writer wins across channels/ports targeting that synth. Physical ownership
overrides incoming MIDI only for its captured track. CC120/121/123 reset modulation;
this does not implement new sustain or all-notes-off ownership semantics.

The wheel adds `lfo * amount >> 11` to the existing 1/4096-semitone pitch signal:
maximum approximately ±0.5 semitone. It uses the existing waveform/rate/fade and
does not modify saved PIT depth or project/parameter formats. The additional term
is calculated once per track block, then shared across its voices. Default zero
leaves the existing pitch calculation unchanged. No generated MIDI pitch bend or
automatic outgoing CC1 is added; notes/velocities/arp timing are unchanged.

STOP clears all wheels. Track panic clears that synth only; drum panic does not
clear synth wheels. USB reset/detach clears values whose latest owner was USB,
retaining TRS values. TRS unplug cannot be detected; send CC1 zero/reset or STOP.
Runtime state is bounded, with no allocations or new DSP buffers.

Android uses a single-pointer `ModulationPlayer` to emit/deduplicate CC1 and restore
zero on release/cancel. Perform's horizontal strip reuses the established touch
surface and lifecycle cleanup, routed by synth or generic channel. Strip and XY
macro acquisition are mutually exclusive. The strip is not persisted as a patch
setting or recorded into note-only Perform capture. External-controller input through
the phone does not forward CC1 yet; direct FM1 MIDI and app CC automation can send it.

Native production-control tests exercise priority/restoration, parameter preservation,
bounds, held-note gates, real USB/TRS packets, reset ownership and STOP/panic cleanup.
Domain tests exercise pointer ownership, deduplication, routing, zero reset, validation
and send failures. Hardware audible/gesture validation remains user testing.
