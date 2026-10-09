# Protocol boundary

Platform-neutral SLOOP SysEx stream parser, signed values/pack7 codec, INFO/DESC decoders,
and serialized request client. Android supplies the raw-byte MIDI transport.

One request is in flight; push notifications are routed separately. Timeouts or cancellation
after request setup fault the connection epoch to prevent an ambiguous late reply being used
for a subsequent operation. Reconnect before issuing another request. Mutations are not retried.

Typed operations include device metadata/state, WATCH, sample transfer, native patterns,
FM6 exchange and persistent banks, remote performance, drum grooves and USB return controls.
Protocol-12 extensions negotiate support; generic MIDI performs no SLOOP queries.
Atomic scene command 72 remains a tested client/staging scaffold disabled in production.
See [performance wire](PERFORMANCE-WIRE.md), [sound banks](SOUND-BANKS.md),
[groove design](../../../firmware/DRUM-GROOVES-DESIGN.md) and
[editor protocol](../../../web/EDITOR_PROTOCOL.md).
