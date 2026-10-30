# Firmware design and implementation

Current native firmware features and their implementation contracts live here;
`firmware/` holds source. Generated installer firmware packages also live here;
the website installer references the current MerthSloop package.

- [Illustrated guide to all MerthSloop firmware additions](MERTHSOFT-FEATURES.md)
- [Native UI refresh, actual screen gallery and design](UI-REFRESH-DESIGN.md)
- [Ambient DRIFT, DRONE, note EVOLVE and sustained starters](AMBIENT-PERFORMANCE-DESIGN.md)
- [Adjustable voice budget and drone recovery](VOICE-BUDGET-EXPERIMENT.md)
- [Track sustain: pedal, piano, strings and LOFI](SUSTAIN-DESIGN.md)
- [Drum-triggered ducking: source, recovery and shape](DUCKING-DESIGN.md)
- [Cold UI and engine-table headroom measurements](COLD-UI-HEADROOM.md)
- [Compact parameter descriptors and measured flash savings](PARAMETER-DESCRIPTOR-COMPACTION.md)
- [Explicit SONG SAVE and power-on working-state persistence](SONG-SAVE-PERSISTENCE.md)
- [Reverse delay and swelling lead sounds](REVERSE-SOUNDS.md)
- [Combined UI reuse audit integration](UI-REUSE-INTEGRATION.md)
- [More NES, Game Boy, Mega Drive and worn-digital presets](CONSOLE-SOUNDS-EXPANSION.md)
- [Musical sequence starters](SEQUENCE-STARTERS-DESIGN.md)
- [Custom progressions](CUSTOM-PROGRESSIONS-DESIGN.md)
- [Phone musical starter protocol](MUSICAL-STARTER-PROTOCOL.md)
- [Arpeggio expression](ARP-EXPRESSION.md)
- [Live vibrato, tremolo and MIDI modulation](MODULATION-WHEEL.md)
- [Instrument and console-inspired preset pack](INSTRUMENT-PRESET-PACK.md)
- [LOFI chord arpeggios](LOFI-CHORD-ARP.md)
- [LOFI bounded attack pitch sweep](LOFI-SWEEP.md)
- [Live track auto-wah and auto-pan](AUTO-WAH-DESIGN.md)
- [Combined sound-engine performance integration](ENGINE-PERFORMANCE-INTEGRATION.md)
- [Master LOFI processing optimization and validation](LOFI-PERFORMANCE.md)
- [VOICE/LOFI dropout investigation: shedding, scheduling and transition work](VOICE-LOFI-FINDINGS.md)
- [Held-note recovery and measured render scheduling](VOICE-LOFI-FIXES.md)
- [Reverb types and dual-path spring](REVERB-EXPANSION.md)
- [Selective upstream fixes and piano-roll integration status](UPSTREAM-2.8-INTEGRATION.md)
- [Upstream controls: integrated FX/SEQ preferences and remaining work](UPSTREAM-CONTROLS-REVIEW.md)
- [Upstream legato versus independent note ties](LEGATO-TIES-COMPARISON.md)
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
- [Six-specialist .91 flash audit synthesis and integration priorities](FLASH-91-AUDIT-SYNTHESIS.md)
- [Integrated UI/storage/startup-data savings and measured combined build](FLASH-91-INTEGRATION.md)
- [Ambient headroom audit synthesis and first integrated batch](AMBIENT-HEADROOM-SYNTHESIS.md)
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

- [Measured voice-budget options and hardware qualification](VOICE-BUDGET-AUDIT.md) — audit candidates only.
- [Additional build-time compression audit](SUSTAIN-HEADROOM-AUDIT.md).

- [Track snapshots and morphing](TRACK-SNAPSHOTS-DESIGN.md): native A/B controls, scope, lock handling and lifecycle.
- [Shared modulation waveforms](MODULATION-WAVEFORMS.md): centralized control waves and shaped DRIFT/MORPH.
- [Drum loudness audit](DRUM-LOUDNESS-AUDIT.md): measured hit balance, dense-groove kick truncation and recommendations.
- [Drum playback integration](DRUM-PLAYBACK-INTEGRATION.md): upstream allocator and three measured snare edits.

- [Sound explorer and reversible auditions](SOUND-BROWSER-DESIGN.md).
- [Bounded startup literal storage](LITERAL-STORAGE.md).
