/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef SLOOP_SCENE_TRANSACTION_H
#define SLOOP_SCENE_TRANSACTION_H
#include <stdint.h>
#define SC_TX_COMMAND 72u
#define SC_TX_MAX 3072u
#define SC_TX_CHUNK 96u
#define SC_TX_SCHEMA_SIZE 2956u
enum { SC_CAPS, SC_BEGIN, SC_DATA, SC_PREPARE, SC_COMMIT, SC_CANCEL, SC_STATUS };
enum { SC_OK, SC_INVALID, SC_UNSUPPORTED, SC_BUSY, SC_CONFLICT, SC_MISSING, SC_TOO_LATE };
enum { SC_IDLE, SC_RECEIVING, SC_PREPARED, SC_QUEUED, SC_APPLIED, SC_CANCELED, SC_FAILED };
/* All callbacks must be installed together before advertising live support.
 * apply runs at the engine boundary: prevalidated, no allocation/flash/USB/failure.
 * critical protects only bounded descriptor operations, never CRC/validation.
 * revision includes ALL local/remote pattern, sound, tempo and dependency edits. */
typedef struct {
    uint32_t (*revision)(void);
    int (*validate)(const uint8_t *, uint32_t);
    void (*apply)(const uint8_t *, uint32_t);
    void (*enter)(void);
    void (*leave)(void);
    int (*schedule)(uint32_t, uint8_t); /* validate future exact beat/bar/phrase tick, bounded */
} sc_tx_hooks;
typedef struct {
    uint8_t payload[SC_TX_MAX];
    uint32_t epoch, revision, crc, boundary, applied_revision;
    uint16_t token, last_token, length, received;
    volatile uint8_t state;
    uint8_t reason, boundary_kind;
} sc_tx_state;
/* Schema: v1, tempo u14, mask=7; three ordered track records:
 * FM6 packed[128], seven signed14 macros, length, division;
 * 64 * (step_t[10], micro+32), fill[16], 24*(step,param,signed14, reserved=0).
 * raw little-endian bytes, chunk transport uses pack7. No sample or drum data. */
static int sc_tx_schema_valid(const uint8_t *p, uint32_t n);
#endif
