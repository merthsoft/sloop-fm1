# Selective upstream 2.8 integration

This batch adapts the low-conflict fixes from upstream commit beab3f62d04ff8defcc7b663505bd6bd6ad7836a, reviewed against the last integrated 2.7.1 base (4ec1ac8). Main/origin main mirror upstream at b77bf55; merthsloop is not marked as fully merged with 2.8. Its version remains based on 2.7.1 with an incremented MerthSloop build.

## Included

- Early USB RESCUE scan drives PA2/backlight low before scanning OCT−, using the upstream HAL helper. Real register access remains in hal/.
- Encoder detent counting no longer requires each intermediate state to survive two scan frames. Existing bounce, reverse, skipped-state and rest handling remains; the UI's acceleration preference is unchanged.
- USB Audio Class lists audio streaming before MIDI in the audio-control collection and bumps bcdDevice by 0.08 when audio is enabled. All collection members/interfaces/endpoints and CDC/rate variants are otherwise retained. No USB playback return or queue-size change is imported.
- MIDI clock tempo estimation averages up to eight beats, uses 0.8 BPM hysteresis after filling the window, and resets the history for a change of at least five BPM or clock reacquisition. Musical phase still follows pulses. Existing USB/TRS source filtering, dropout handling, eighth swing and HALF/DOUBLE internal-clock policy remain.
- Browser updater discovers generic Windows loader names by the verified FM-1 handshake, prefers matching port pairs, and waits up to 60 seconds for handoff/reboot. Normal firmware discovery retains its named-port policy. Existing foreign-loader protection and return-to-stock support are retained; upstream's removal of return-to-stock is not included.

## Not included

No whole-file replacement or upstream merge was performed. Recording/legato bit 7, preset-owned arp behavior, resonance, punch fade, wholesale UI replacement and sample-content cuts remain separate candidates. CC11/CC0 support and the native NOTES/action sheets are now integrated through MerthSloop-specific helpers. The new ROOM/PLATE/HALL and dual-path SPRING work remains intact. Existing settings/project prefixes are unchanged by this backport.

The piano roll preserves independent ties and modifier gestures; see PIANO-ROLL-INTEGRATION.md for implemented controls and remaining recording compatibility work. The initial candidate ranking is in UPSTREAM-CANDIDATES.md.

## Verification

Recovery boot-key ordering and fast-encoder regressions pass. The complete
sequencer regression passes, including upstream's minute-long jitter, half-BPM,
tempo ramp and jump cases. Nine CDC/rate descriptor variants and a compile-time
44.1-only build pass. The latter exposed an old test assumption that SET_CUR
could select 48 kHz even when compiled out; its expectation now respects the
advertised capability.

All 182 audio goldens are unchanged. Swing, HALF/DOUBLE combinations, durable
section/startup saves and the canonical upstream project fixture pass. Native
and WASM audio match across all engines and five reverb models. Hot DSP target
budgets and inlining checks pass. The full web/update suite passes, including
generic-loader install/resume, interrupted handoffs, foreign-loader rejection,
matching-port behavior and retained return-to-stock simulations.
The web parity test also caught stale OCT UP/MONO labels for our existing
DOUBLE TIME/VINYL slots; the web controls now match the native effect IDs.

Firmware 2.7.1.36: image 580,304 bytes, 1,260 bytes free; main static RAM
85,688/98,304, low RAM 22,772/29,496, pool 332,876/344,064. Relative to the
reverb build, the selective backports add 192 image bytes and 32 static RAM
bytes. No audio buffer is added and the reverb algorithms are unchanged.

Host checks cannot establish physical rescue availability, fast-knob behavior,
USB enumeration on every host or real audio deadline margin. These remain
hardware validation points; no automatic firmware flash is performed.

## Editor and MIDI integration completion

The subsequent integrated build also adapts the piano-roll visual/editing ideas
while retaining native independent ties, chord merges, library pages and controls.
CC11 expression and bank-aware Program Change are integrated through actual drum
audio and user-record loading. The fork still is not a full 2.8 merge: upstream
POLY recording/bit-7 legato semantics remain deferred. Current production image
is 581,140 bytes (424 free), main static RAM 89,064/98,304. See
[editor integration](PIANO-ROLL-INTEGRATION.md) and [MIDI](MIDI-EXPRESSION-BANKS.md).
