# Hardware, USB and updater review

Reviewed 2026-10-23: current working tree based on HEAD `9fd9ddc`, including the uncommitted icon cleanup. Scope was read-only except this report and private evidence in `build/code-review-hardware-51/`. No production source, generated output, Git refs or hardware were changed. No repository AGENTS.md was found. Existing OPTIMIZATION, ARCHITECTURAL-HEADROOM and REDUNDANCY-AUDIT results are not counted as new savings.

## Integration recommendation

Two confirmed updater completion defects merit small follow-up fixes, with the erase-error handling first. Neither reproduction writes physical flash. Preserve the current app/storage boundaries and audio/USB ownership design. This review produced no target-measured optimization and makes no image-size or timing claim for candidate fixes. Rebuild the actual loader/package and rerun interruption tests after integrating either fix.

## Ranked confirmed findings

### 1. P2 — Record erasure failure is reported as successful completion

**Location:** `firmware/loader/ldr_core.c:92-103`, especially line 101; callers at lines 124 and 175; reset decision in `firmware/loader/loader.c:98-102`.

`ldr_records_drop()` clears the RAM record first, ignores `ldr_erase()`'s result, and silently breaks on a record-read failure. `ldr_pass()` consequently returns success even if the valid persistent update record remains. Trigger: the app has verified and the host has answered the final request, but the 0xE4000 record-sector erase fails (write protection, timeout or device fault). Consequence: the loader resets claiming completion while the SPL still has its valid flash update record, so it can reenter the loader rather than boot the verified application. This also affects the unknown-chip exit intended to return to the old firmware. This is a completion/recovery failure, not evidence of erased app/head data.

**Reproduction:** the private copy of `tests/ldr_test.c` changes only its NOR hook to return -4 without changing bytes when erasing 0xE4000. The actual `ldr_session()` returns **0** on its first pass, but the existing assertion “update record cleared (RAM + flash)” fails. The baseline with the same package passes. Evidence: `build/code-review-hardware-51/validation.log`, `snapshot/tests/ldr_erase_fail.c`, executable `ldr-erase-fail`.

**Why current tests miss it:** `tests/ldr_test.c:91-99` always successfully erases a sector; its otherwise useful persistent-record assertions therefore never test this error branch. Package corruption and no-final-response cases do not exercise flash cleanup failure.

**Minimal fix:** return a status from `ldr_records_drop()`, propagate read/erase failures, and verify that each matching record was invalidated before returning success or LDR_EFLASH. Keep the loader available for retry on failure. Clear the RAM record only after successful persistent invalidation for this completion path (the separate early-start RAM clear serves its own warm-reset policy). Do not weaken bounds or erase broader ranges.

**Regression:** inject an erase failure and an apparently successful erase that leaves bytes unchanged; assert no successful completion/reset, record remains resumable, and the next session succeeds after the fault is removed. Add a record-read failure case and exercise both ordinary completion and unknown-chip exit.

### 2. P2 — A negative final host reply is accepted as success

**Location:** `firmware/loader/ldr_core.c:170-175`.

The final 0xF0000000 read accepts any correctly framed eight-byte body. It never checks the promised `success\0` payload before dropping the update records. Trigger: the host answers that address with a well-formed negative/error body such as `failure\0`. Consequence: despite host refusal, the loader discards its resume state and boots the installed app. The image CRC has already passed, so this does not bypass package-integrity validation, but it violates the completion handshake. The stage-one equivalent already compares all eight bytes (`firmware/src/ota.c:347-349`), and `tools/fm1_install.py:296` emits `success\0` for the normal final reply.

**Reproduction:** private fake-host replacement serves `failure\0` only at 0xF0000000, preserving the response address, length and checksum. The actual loader returns **0**, clears the record, and the entire original loader suite still exits **0**. Evidence: `snapshot/tests/ldr_bad_finish.c`, executable `ldr-bad-finish`, `validation.log`.

**Why current tests miss it:** the fake host either emits literal success (`tests/ldr_test.c:60-64`) or sends no reply (`:55-58`). Its missing-response regression checks timeout, not response semantics.

**Minimal fix:** require `ota_memeq(sec, (const uint8_t *)"success", 8)` alongside successful `ota_read`, as the stage-one path does; exhaust the bounded retries and preserve records otherwise.

**Regression:** send a checksum-valid eight-byte negative reply and assert LDR_ENOFIN plus retained RAM/flash completion state; then send success on a subsequent attempt/session and assert normal completion. Retain existing dropped-response coverage.

## Coverage map and clean areas

| Inspected files/includes | Review focus and result |
| --- | --- |
| `firmware/src/felucca.c`, `main.c`, `audio.c` | Single-unit integration, flash wrappers, OTA scratch lifetime, startup order and nested TIMER5/ALNK0 control flow. TIMER5 owns normal USB/TRS production; audio consumes MIDI and produces UAC. Normal USB polling is deferred during audio; nested work is confined to EP4. No additional confirmed defect found in that ownership model. |
| `firmware/src/usb.c`, `uac_fir.h` interface; `hal/fm1_usb.h`, `fm1_cc.h` | Descriptors and rate modes, EP0 transfer abandonment/data stages, EP1 bounds/backpressure, SysEx collection/TX, CDC rings, UAC priming/resampling/consumer. Packet buffers and ring limits are explicitly bounded. EP4 has separate DMA slots and sets INDEX before use. Bus reset clears pending EP0 class receive state. MIDI backpressure reserves TRS headroom. See validation and limits below. |
| `firmware/src/midi_uart.c`, `hal/fm1_uart.h` | Running status, realtime inside SysEx, system-status cancellation, sentinel DMA ring and shared MIDI producer. USB and TRS enqueue from one serialized TIMER5 context; the content sentinel design is intentional and addresses an established hardware count-latching problem. No speculative change to that design proposed. |
| `firmware/src/ota.c`, `loader/ldr_core.c`, `loader/loader.c`, `loader/crt0_ldr.S`, `loader/loader.ld` | Header/frame lengths, staged-loader identification/CRC/readback, stage head written last, persistent record, app-only sector writes, retry and finish, RAM-only loader execution. Findings above. Physical app erase range remains 0x4000..0x93000; flash head excluded. Staging remains 0xE0000..0xE5000. |
| `firmware/src/bootguard.h`, `recovery.c`, `main.c` | Retained boot-loop guard, recovery-to-ROM fallback, physical-key setup, watchdog and polled recovery USB. Recovery explicitly depends on intact application startup; it is not an independent boot bank. Existing backlight-before-key-scan fix is preserved. |
| `firmware/src/cpu1.h`, `cpu1.c`, relevant `fx.c` dispatch/wait; `hal/fm1_cpu1.h`, relevant `fm1_sys.h` | Single-job publication/completion, volatile control words and csync barriers, RAM waiting loop, boot self-test, core hold on timeout, explicit partial-job fallback. Existing 8 ms timeout exceeds the normal audio half deadline and intentionally allows a glitch; no claim that this path is deadline preserving. |
| `hal/fm1_flash.h`, `fm1_xip.h` interface | Overflow-resistant FL_IN range checks, page-program bounds, IRQ-off flash windows, RAM-only routines and cache invalidation. The loader has its own write-range override. No partition expansion or weakening proposed. |
| `hal/fm1_irq.h`, `fm1_isr.S`, `fm1_vec.S`, `fm1_audio.h`, `fm1_timer.h`, `fm1_time.h`; portions of `fm1_input.h` | Interrupt priorities, wrapper register save/restore, fatal frame shape, timer wrap arithmetic, DMA buffer ownership, input edge-consumer critical sections. Chip-specific instruction/cache/register behavior was inspected as existing platform assumptions, not independently proven. |

## Validation performed

`build/code-review-hardware-51/reproduce.py` copies the inspected firmware/test sources into its private snapshot and compiles with the repository `build/host/optimization-cc` wrapper (Zig 0.13, -O2), with outputs exclusively in that private directory. Package fixture: `docs/firmware/merthsloop-2.7.1.49.fwsc`, used as both old/new fixture for the loader boundary/error tests; this is the existing package, not a newly built current-tree image.

- Original loader suite: PASS, including CRC/retry/foreign-key/missing-finish/record retention and head/range assertions.
- Negative-final-body mutation: original suite still PASS, demonstrating the missing semantic assertion.
- Record-sector erase-failure mutation: expected FAIL; first loader pass still reports rc 0, persistent-record assertions fail.
- Current copied UAC suites: PASS for AUTO with CDC built but hidden, and forced 48 kHz with CDC shown, HALF_FRAMES=128.
- Current copied MIDI UART/backpressure suite: PASS.
- Current copied recovery/bootguard suite: PASS.
- Current copied OTA staging suite: PASS against the existing package fixture.

All compiler and test output is retained in `validation.log`. No production candidate patch was applied and no JieLi target rebuild was run.

## Test quality and unmeasured opportunities

The UAC suite parses actual descriptor bytes like a host and exercises packet ordering, fill control, restarts, underrun and overrun; these are useful behavioral checks. MIDI tests exercise actual EP1 ring backpressure, not merely parser implementation details. Loader tests execute the real portable updater against a fake NOR and verify head preservation and retry outcomes. These are strong foundations.

Coverage remains weaker at peripheral sequencing: `tests/uac_test.c` explicitly never calls `uac_service`/`usb_poll`; the inspected scene USB test models registers for SysEx interception but does not validate EP0 setup-abort/status sequencing or nested EP4 service. Extend a peripheral model around those concrete transitions before treating them as tested. Recovery tests substitute OTA and MMIO hooks, so their success proves recovery control flow, not enumeration or interrupt masking on silicon.

`tests/cpu1_host.c:97-98,118-122` models a timeout as a job that never starts. Actual hardware can be halted after partial state mutation; `fx.c:1283-1288` explicitly documents the resulting glitch and reruns units. A meaningful future fault test should halt after one completed unit or partway through a stateful unit, then check subsequent blocks remain usable. This is a test opportunity, not a new finding that intentional glitch tolerance is a bug.

No credible additional flash/speed saving was measured here. Cold USB/OTA helper sharing may be benchmarked privately, but code similarity across mutually exclusive loader/application builds is not duplicated active image cost. Preserve direct DSP inlining and audio deadlines; do not infer savings from source length.

## Explicit limits

This is targeted source review plus host evidence, not proof of absence of bugs. No physical device, electrical UART/LED timing, USB analyzer, flash power interruption, SPL execution, cache-coherency stress or target cycle measurement was available. The raw SFC/SPI timeout recovery behavior, processor ABI, MMIO semantics and core-hold bus-trap workaround remain platform assumptions requiring hardware validation. Input/LCD/ADC/GPIO/guard internals outside the portions named above were not exhaustively reviewed. Factory content, synth algorithms, project-format compatibility and host UI were outside this ownership scope. Existing architectural-headroom hardware gates still apply; none was discharged by these tests.
