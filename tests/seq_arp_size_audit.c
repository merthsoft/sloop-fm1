/* Compile only (no firmware link): exact target ABI cost of production state. */
#include "../firmware/src/core.h"
#include "../firmware/src/harmony_owners.h"
_Static_assert(sizeof(harmony_seq_source) == 6, "sequence owner target size");
_Static_assert(sizeof(harmony_seq_source) * NPART == 18, "sequence owner target RAM");
_Static_assert(sizeof(step_t) == 10, "literal step layout unchanged");
harmony_seq_source seq_arp_audit_state[NPART];
const uint32_t seq_arp_audit_sizes[] = {
    sizeof(harmony_seq_source), sizeof(seq_arp_audit_state),
    sizeof(track_t), sizeof(step_t), P_COUNT, P_E0
};
