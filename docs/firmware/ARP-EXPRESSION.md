# Arpeggio expression

Implemented in `firmware/src/seq.c`; no new parameter, protocol ID, mode ID,
project/preset field or persistent storage is introduced.

Live MIDI input retains its per-note velocity in the existing 16-note held pool.
Manual keyboard and chord modifier additions remain velocity 100. A repeated live
note-on updates that held pitch to the latest attack velocity; it retains the
existing physical-count/release policy. This is not a new per-port/channel owner
system: multiple live owners of one pitch still share the legacy held entry.

SNOTE/SPLAY sequence publication snapshots `step_vel` for each literal pitch:
default base 96, stored step base otherwise, accent base 127, then the existing
per-note two-bit level mapping. Duplicate pitches within a step use the maximum.
The sequence snapshot survives TIE unchanged. REST, rejected fill, STOP, panic,
and project adoption clear membership through existing paths, making stale velocity
slots inaccessible. HOLD retains only live expression; a new latched chord replaces
the previous live list and its velocities. Removing live or sequence membership
reveals the surviving source's expression.

For a pitch shared by live and sequence sources, the strongest current velocity
wins, even when the 16-root pool is full. Pool capacity and ordering remain live
first, then sequence. Octave expansion carries each root's velocity alongside its
pitch through sorting and every selector. Single-note modes preserve the expression
of the selected expanded entry when distinct roots overlap in an octave. PULSE
deduplicates all expanded pitches (including nonadjacent PLAY-order entries and
127 clamping) and uses the maximum velocity of their contributors.

Audio `trk_note_on`, generated MIDI `seq_out_on`, and recording `rec_note` receive
the same resolved velocity. Recording still applies the existing velocity-to-level
quantization; it does not introduce lossless MIDI expression storage. Audio engines
retain their own velocity response, voice allocation and unison scaling. The eight
voice budget can still steal PULSE tones. Probability comparison and random draws,
selector cycles, rate, swing and gate calculations are unchanged.

Route changes transfer actual pitch/velocity snapshots between direct and routed
playback, including ties, rather than reconstructing expression from potentially
edited sequence storage. Direct ratchet additions also cache their attack velocity.
Physical/MIDI note-on first invokes the existing optional `sequence_preview_end`
callback, allowing the musical-library preview to release before live input starts.

## Resource and integration notes

Three bounded byte arrays add exactly 72 static bytes: live 3x16, sequence 3x4,
direct sounding snapshot 3x4. A production static assertion enforces this budget.
`track_t`, `step_t`, and the existing six-byte sequence owner are unchanged. Against
the supplied 956-byte headroom baseline, nominal headroom is 884 bytes before final
link/alignment and other concurrent changes. Parent must measure the combined link.

Selectors and expression pool construction remain out of line. Each caller's
parallel velocity buffer adds 64 bytes of local array storage; route transfer adds
four bytes. No allocation or queue is introduced. Existing bounded insertion sorts
move velocities with pitches. Source merge scans at most 4x16 entries. PULSE scans
at most 4096 entries across duplicate checking and maximum selection for its existing
64-entry pool; it never rescans MIDI pitch space or persistent sequence history.
Target stack high-water and real ISR deadline remain integration checks.

The focused real-core test is `tests/seq_arp_expression_test.c`. Add to the parent
runner (using its existing CC, OUT and run variables):

```sh
$CC -UNDEBUG -O2 -w -Ibuild/gen -Ifirmware/src -Ifirmware/hal -o "$OUT/seq_arp_expression_test" tests/seq_arp_expression_test.c -lm
run "arpeggio expression: source velocity, octave overlap, latch, routes and recording" "$OUT/seq_arp_expression_test"
```

The same test supports the runner's existing integer-divide-by-zero sanitizer
convention. No translation unit registration or public API change is needed.
Internal integration points are `input_on`, `seq_step`, `seq_step_velocity`,
`arp_expression_list`, `arp_next_velocity`, and `arp_emit`. Compatibility wrappers
retain `arp_add` (manual velocity 100), `arp_list`, and noinline `arp_next` for existing
host tests; production velocity selection is also noinline.

Validation in this checkout: optimized expression, seq2, seq_arp_modes and seq_arp
tests pass with Zig 0.13 host cc. Expression also passes an integer-divide-by-zero
trap build. Zig's default recover/abort sanitizer runtime failed to link
`__ubsan_handle_divrem_overflow_abort`; trap instrumentation checks the same division
sites without changing DSP sources. Compile-only pi32v2 ABI probe confirms the
sequence owner remains 6 bytes (18 total), current `track_t` is 2076 bytes, `step_t`
10 bytes, P_COUNT 61 and P_E0 53. This component changes no `track_t` fields; the
current sizeof differs from the older design checkpoint because of shared work.
No combined image, release package, hardware acceptance or ISR deadline claim is
made by these host tests and isolated ABI checks.
