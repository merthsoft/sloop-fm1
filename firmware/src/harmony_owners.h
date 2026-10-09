#ifndef FM1_HARMONY_OWNERS_H
#define FM1_HARMONY_OWNERS_H
#include <stdint.h>

/* Caller-owned, transient state. No audio voices, persistence, allocation or latch policy.
 * One instance per destination track; MIDI identity includes the physical port.
 * Mutation must be serialized by the caller. Observers run only after return. */
#define HARMONY_OWNER_CAP 16u
#define HARMONY_TONE_CAP 4u
enum harmony_source { HARMONY_LOCAL, HARMONY_USB, HARMONY_TRS, HARMONY_SEQ };
/* Production sequence/live boundary: one exclusive sequencer owner per track.
 * Live keys retain their existing held-list policy. No transient state persists. */
typedef struct { uint8_t n, note[4], routed; } harmony_seq_source;
typedef struct { uint8_t source, channel, trigger; } harmony_owner_id;
typedef struct {
    harmony_owner_id id;
    uint8_t active, count, pitches[HARMONY_TONE_CAP];
} harmony_owner;
typedef struct {
    harmony_owner owners[HARMONY_OWNER_CAP];
    uint8_t membership[128];
    uint32_t rejected;
} harmony_owners;
/* Aggregate transitions only, sorted ascending. Common tones never retrigger.
 * Emit all off[] before on[]. The complete new state is already committed. */
typedef struct { uint8_t off_count, on_count, off[4], on[4]; } harmony_delta;
void harmony_owners_init(harmony_owners *state);
int harmony_owner_replace(harmony_owners *state, harmony_owner_id id,
                          const uint8_t *pitches, unsigned count, harmony_delta *delta);
void harmony_owner_release(harmony_owners *state, harmony_owner_id id, harmony_delta *delta);
/* Panic clears every owner and aggregate pitch; caller releases active output
 * via its existing panic path, since a four-tone delta cannot describe all owners. */
void harmony_owners_clear(harmony_owners *state);
#endif
