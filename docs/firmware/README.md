# Firmware design and implementation

Current native firmware features and their implementation contracts live here;
`firmware/` holds source. Generated installer firmware packages also live here;
the website installer references the current MerthSloop package.

- [Illustrated guide to all MerthSloop firmware additions](MERTHSOFT-FEATURES.md)
- [Native UI refresh, actual screen gallery and design](UI-REFRESH-DESIGN.md)
- [Musical sequence starters](SEQUENCE-STARTERS-DESIGN.md)
- [Custom progressions](CUSTOM-PROGRESSIONS-DESIGN.md)
- [Phone musical starter protocol](MUSICAL-STARTER-PROTOCOL.md)
- [Arpeggio expression](ARP-EXPRESSION.md)
- [Live vibrato, tremolo and MIDI modulation](MODULATION-WHEEL.md)
- [LOFI chord arpeggios](LOFI-CHORD-ARP.md)
- [Live track auto-wah and auto-pan](AUTO-WAH-DESIGN.md)
- [Reverb types and dual-path spring](REVERB-EXPANSION.md)
- [Selective upstream fixes and piano-roll integration status](UPSTREAM-2.8-INTEGRATION.md)
- [Native piano roll and action sheets](PIANO-ROLL-INTEGRATION.md)
- [Master LOFI characters](LOFI-MASTER-DESIGN.md)
- [MIDI expression and preset banks](MIDI-EXPRESSION-BANKS.md)
- [Recording snap and STEP editing](SEQUENCER-EDITING.md)
- [Independent per-note ties](PER-NOTE-TIES-DESIGN.md) — native editing and playback implemented; recording/import follow-on
- [Persistent keyboard scale lights](KEYBOARD-SCALE-LIGHTS.md)
- [Drum grooves](DRUM-GROOVES-DESIGN.md)
- [Rhythm shaping](RHYTHM-SHAPING-DESIGN.md)
- [Eighth-note swing grid](SWING-GRID-DESIGN.md)
- [Chord and arpeggio architecture](CHORD-ARPEGGIO-DESIGN.md) — includes future work
- [Native inversion and spread controls](CHORD-VOICING-DESIGN.md)
- [Progression performance design](PROGRESSION-PERFORM-DESIGN.md) — integrated arp-driven local pads; queued changes and starter rhythms deferred
- [Punch FX and superseded modifier design](PUNCH-FX-DESIGN.md)
- [USB playback](USB-PLAYBACK-IMPLEMENTATION.md)
- [Lossless font packing](FONT-PACKING-DESIGN.md)
- [Measured branch optimization](OPTIMIZATION.md)
- [Architectural headroom investigation and ranked next steps](ARCHITECTURAL-HEADROOM.md)
- [Measured MerthSloop/main flash and RAM headroom](HEADROOM-COMPARISON.md)
- [Duplicated-code audit and measured UI consolidation](REDUNDANCY-AUDIT.md)
- [Optimization candidates worth suggesting upstream](UPSTREAM-OPTIMIZATION-CANDIDATES.md)
- [Visualizers and screenshot gallery](VISUALIZERS.md)

Use the [player guide](../../GUIDE.md) for controls and the
[current status](<../../android/design docs/STATUS.md>) for the integrated feature list.

- [Scale-aware grids and MIDI keyboard following](SCALE-MIDI-GRIDS.md)

- [Upstream 2.6 integration assessment](UPSTREAM-2.6-INTEGRATION.md)
- [PR 111 RAM and dual-core adoption assessment](PR-111-ASSESSMENT.md)

- [Upstream 2.7.1 assessment and compatibility gates](UPSTREAM-2.7-INTEGRATION.md)

- [Static-analysis audit](STATIC-ANALYSIS.md): unused paths, measured cleanup, optional-build checks and remaining candidates.

- [Screen-by-screen UI polish audit](UI-POLISH-AUDIT.md) — findings, changes and verification.

- [DOUBLE TIME transport and VINYL punch routing](DOUBLE-TIME-DESIGN.md).
