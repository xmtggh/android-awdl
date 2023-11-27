#include <time.h>
#include <string.h>

#include "version.h"
#include "state.h"

#define ETHER_BROADCAST (struct ether_addr) {{ 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }}
#define PSF_INTERVAL_MASTER_TU 110
#define PSF_INTERVAL_SLAVE_TU 440

void apple_p2p_init_state(struct apple_p2p_state *state, const char *hostname, const struct ether_addr *self,
                     struct apple_p2p_chan chan, uint64_t now) {
	state->self_address = *self;
	strncpy(state->name, hostname, HOST_NAME_LENGTH_MAX);
	state->version = apple_p2p_version(3, 4);
	state->dev_class = APPLE_P2P_DEVCLASS_MACOS;

	state->sequence_number = 0;
	state->psf_interval = PSF_INTERVAL_MASTER_TU;
	state->dst = ETHER_BROADCAST;

	state->tlv_cb = 0;
	state->tlv_cb_data = 0;

	state->peer_cb = 0;
	state->peer_cb_data = 0;

	state->peer_remove_cb = 0;
	state->peer_remove_cb_data = 0;

	state->filter_rssi = 1;
	state->rssi_threshold = RSSI_THRESHOLD_DEFAULT;
	state->rssi_grace = RSSI_GRACE_DEFAULT;

	apple_p2p_sync_state_init(&state->sync, now);

	state->channel.enc = APPLE_P2P_CHAN_ENC_OPCLASS;
	state->channel.master = chan;
	state->channel.current = CHAN_NULL;
	//apple_p2p_chanseq_init(state->channel.sequence);
	apple_p2p_chanseq_init_static(state->channel.sequence, &state->channel.master);

	apple_p2p_election_state_init(&state->election, self);

	apple_p2p_peer_state_init(&state->peers);

	apple_p2p_stats_init(&state->stats);
}

void apple_p2p_stats_init(struct apple_p2p_stats *stats) {
	stats->tx_action = 0;
	stats->tx_data = 0;
	stats->tx_data_unicast = 0;
	stats->tx_data_multicast = 0;
	stats->rx_action = 0;
	stats->rx_data = 0;
	stats->rx_unknown = 0;
}

uint16_t apple_p2p_state_next_sequence_number(struct apple_p2p_state *state) {
	return state->sequence_number++;
};

void ieee80211_init_state(struct ieee80211_state *state) {
	state->sequence_number = 0;
	state->fcs = 0;
}

unsigned int ieee80211_state_next_sequence_number(struct ieee80211_state *state) {
	uint16_t seq = state->sequence_number;
	state->sequence_number += 1;
	state->sequence_number &= 0x0fff; /* seq has only 12 bits */
	return seq;
};

uint64_t clock_time_us() {
	int result;
	struct timespec now;
	uint64_t now_us = 0;

	result = clock_gettime(CLOCK_MONOTONIC, &now);
	if (!result) {
		/* TODO why using 'long' for time? */
		now_us = now.tv_sec * 1000000;
		now_us += now.tv_nsec / 1000;
	}
	return now_us;
}
