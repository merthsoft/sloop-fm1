# Bounded startup literal storage

The target build pools complete string-literal objects in flash and restores a
bounded subset into RAM once at boot. This funds the sound explorer without
removing instruments/features or adding decoding to audio playback.

## Build contract

tools/pool_literals.py operates on the unlinked JieLi ELF32 object after preset
probing/compilation and before final linking. Only .rodata.str1.1 object symbols
are transformed. Complete bytes, sizes, terminators and embedded zeros are
preserved. Longest roots are collected deterministically, then assigned in lexical order into at
most 5,500 bytes main RAM, 2,500 bytes pool RAM, and the remaining flash literals.
The selected groups are laid out in reverse lexical order, then encoded with a shared byte-pair
grammar before LZ4 packing. Unused high-byte and control-byte values represent frequent adjacent
pairs; original bytes remain literals. Dictionary children always refer to
older symbols, and generation caps depth for an eight-byte expansion stack.

Each literal symbol points directly to its new pool offset. Existing relocations
remain for the linker to resolve; no linked addresses are serialized. Unknown
section/symbol layouts, relocation forms/types or nonzero addends fail the build.
The flash remainder is already pooled and keeps explicit symbol offsets rather
than requesting a second linker merge. The retained raw object and generated
oracles/report allow complete comparisons.

The two destinations are NOLOAD ranges in the existing BSS/pool regions. Existing
startup zeroing contains both. boot_literals_init uses the existing bounded boot
decoder and in-place grammar expansion after boot_tables_init and before sample metadata, preset restoration,
UI/storage initialization, interrupts being enabled, or core 1 launching. Failure
returns to ROM recovery. There is no per-note, per-frame or playback decode.
Host/WASM builds retain ordinary literals and do not use the target-object transform.
No project, sample, update-package or wire format changes are introduced.

## Proof and limits

Run python3 tests/pool_literals_test.py for synthetic complete-object/embedded-zero
and refusal cases. After the target build, run it with --target under WSL to
compare every original literal with the transformed object and final linked
address, execute the real C boot decoder three times under UBSan, compare restored
bytes and check destination guard bytes. Normal target region, RAM-code and DSP
budget/inlining checks remain required.

This trades resident RAM for flash and adds startup work. Physical boot duration,
stack high-water and hardware deadline qualification still require device testing.
It does not compress audio or executable code. The older full-string audit's
larger RAM allocation is not adopted; this bounded variant preserves more main
and pool margin. Current linked measurements are in HEADROOM-COMPARISON.md.

## Shared label grammar

The grammar offers unused byte values 128–255 first, then unused control bytes
1–31. Values already present anywhere in the original streams are never used as
tokens; NUL remains literal. High tokens index the dictionary at `v-128`, low
tokens at `v+127`, with explicit zero holes for unavailable codes. Encoder and
decoder changes must land together. Restored RAM limits remain 5,500/2,500 B.
The expanded token range is included in the [combined flash integration](FLASH-91-INTEGRATION.md).

LZ4 first restores the shorter token stream at the start of its final destination.
The grammar expands it backwards in place, right child before left. Every token
emits at least one byte, so unread tokens are never overwritten. The decoder
checks the output/input boundary, final length and bounded stack. Invalid trees
or lengths return failure and enter ROM recovery before normal startup. There is
no recursive C call or new resident scratch allocation.

The same-version complete .82 comparison is **584,224 → 583,372 B**, recovering
**852 B net**, including decoder, metadata, dictionary and layout changes. The
254-byte dictionary plus token/LZ4 streams replace 5,482 payload bytes with 4,476;
the 1,006 B data gain is reduced by 154 B of code/metadata/layout cost. Static RAM
and the existing 4,499 / 2,500 B label destinations are unchanged. The standalone
new restore helper is 182 target bytes; that is not its net image cost.

The .83 version builds to the same 583,372 B, leaving **964 B flash**. Native tests
cover backward overlap, nested grammar, embedded zeros, high-byte literals,
short/long output, invalid token lengths and cyclic trees. All original target
literal objects/final addresses and real boot restoration are checked under
UBSan. The sample blob and 26 other preset/lookup assets match the same-version
baseline byte-for-byte; 200 audio goldens remain unchanged. Hardware startup
qualification remains pending. Evidence and the frozen .82 baseline are under
build/headroom-83; old audit savings are not counted again.

The .85 persistence build raises the main limit from 4,500 to 5,500 bytes;
actual restored main/pool literals are 5,500/2,500 bytes. Including the new
section cache, main static RAM is 97,324 B and pool occupancy is 335,424 B.
The earlier .83 measurements above retain their original 4,500-byte limit.
