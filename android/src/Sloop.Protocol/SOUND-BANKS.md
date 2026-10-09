# FM6 bank storage (existing protocol 9)

No command allocation or firmware schema change. Physical FM6 support and the existing
61-parameter, engine-start-53 layout are required by the Android adapter.

* 70 LIST: no arguments; reply factory count 8, bank count 27, then 35 `(used, zero-terminated name)` entries. Used is 0/1; names are at most ten printable ASCII characters. Empty names are valid for used legacy voices. Empty slots have an empty name.
* 68 GET: `[1, slot]`; reply `[1, slot, rc, packed128?]`. Slots are 0–26. rc 0 requires exactly 128 seven-bit bytes; rc 2 with no bytes means empty/unavailable. Typed patch decoding additionally rejects out-of-range content.
* 69 PUT: `[1, slot, packed128]`; reply `[1, slot, rc]`. rc 0 acknowledged, 1 invalid destination, 2 flash refusal/failure or backup busy, 3 song must stop.

FM6 banks contain base voices, not macros. Projects retain PTCH: factory 0–7, bank 8–34.
Android does not implicitly change PTCH or select a hardware track during saving.

Save prepares a copy of the previous record and named desired record, reviews overwrite, checks
the destination again, writes once and compares exact packed readback. Refusal is distinct from
unknown persistence. EditorClient faults its epoch on timeout/disconnect/cancel-after-send; no
attempt is made to reuse that epoch for recovery or silently retry a flash operation. Reconnect
and explicitly inspect the slot. An rc-0 readback verifies device-visible bank contents, not power-loss
durability beyond the firmware's storage guarantees.

The public protocol lacks a bank generation/CAS token. A simultaneous panel edit after the final
pre-write read can race a write. Android detects observed conflicts and verifies its result but
cannot claim atomic cross-controller overwrite protection.
