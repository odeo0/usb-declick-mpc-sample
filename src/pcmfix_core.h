#ifndef MPC_PCMFIX_CORE_H
#define MPC_PCMFIX_CORE_H
#include <stdint.h>
#define PCM_PERIOD 441u
#define PCM_INSTANT_THRESHOLD 429496730LL /* 0.20 Q31; conservative out-of-phase repair */
#define PCM_CALIB_THRESHOLD 300647711LL /* 0.14 Q31; avoid clean music transients */
typedef struct {
    uint64_t nframes;
    int32_t prev2[2];
    int32_t prev1[2];
    int locked_phase; /* -1 until >= 5 periodic aligned impulses */
    uint64_t corrections;
    uint64_t instant_corrections; /* high-confidence impulses outside current phase */
    uint64_t tracked_corrections; /* interpolation on learned phase */
    uint64_t last_hit_frame;
    uint64_t last_lock_frame;
    uint64_t last_tracking_frame;
    uint32_t hit_streak;
    uint32_t relocks;
    uint32_t phase_moves;
    uint32_t total_events;
} pcmfix_state;
void pcmfix_reset(pcmfix_state *s);
/* Output delayed one frame to allow interpolation of previous frame. */
int pcmfix_step(pcmfix_state *s, const int32_t in[2], int32_t out[2]);
#endif
