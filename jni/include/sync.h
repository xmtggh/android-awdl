#ifndef __SYNC_H__
#define __SYNC_H__

#include <stdint.h>

struct apple_p2p_sync_state {
	uint16_t aw_counter;
	uint64_t last_update; /* in us */
	uint16_t aw_period; /* in TU */
	uint8_t presence_mode;

	/* statistics */
	uint64_t meas_err;
	uint64_t meas_total;
};

void apple_p2p_sync_state_init(struct apple_p2p_sync_state *state, uint64_t now);

uint16_t apple_p2p_sync_next_aw_tu(uint64_t now_usec, const struct apple_p2p_sync_state *state);

uint64_t apple_p2p_sync_next_aw_us(uint64_t now_usec, const struct apple_p2p_sync_state *state);

uint16_t apple_p2p_sync_current_aw(uint64_t now_usec, const struct apple_p2p_sync_state *state);

uint16_t apple_p2p_sync_current_eaw(uint64_t now_usec, const struct apple_p2p_sync_state *state);

int64_t apple_p2p_sync_error_tu(uint64_t now_usec, uint16_t time_to_next_aw, uint16_t aw_counter,
                           const struct apple_p2p_sync_state *state);

void apple_p2p_sync_update_last(uint64_t now_usec, uint16_t time_to_next_aw, uint16_t aw_counter,
                           struct apple_p2p_sync_state *state);

#endif /* __SYNC_H__ */
