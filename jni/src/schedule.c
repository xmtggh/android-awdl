#include "schedule.h"

double usec_to_sec(uint64_t usec) {
	return usec / 1000000.;
}

uint64_t sec_to_usec(double sec) {
	return sec * 1000000;
}

bool apple_p2p_same_channel_as_peer(const struct apple_p2p_state *state, uint64_t now, const struct apple_p2p_peer *peer) {
	int own_slot, peer_slot;
	int own_chan, peer_chan;

	own_slot = apple_p2p_sync_current_eaw(now, &state->sync) % APPLE_P2P_CHANSEQ_LENGTH;
	peer_slot = apple_p2p_sync_current_eaw(now + peer->sync_offset, &state->sync) % APPLE_P2P_CHANSEQ_LENGTH;

	own_chan = apple_p2p_chan_num(state->channel.sequence[own_slot], state->channel.enc);
	peer_chan = apple_p2p_chan_num(peer->sequence[peer_slot], state->channel.enc);

	return own_chan && (own_chan == peer_chan);
}

int apple_p2p_is_multicast_eaw(const struct apple_p2p_state *state, uint64_t now) {
	uint16_t slot = apple_p2p_sync_current_eaw(now, &state->sync) % APPLE_P2P_CHANSEQ_LENGTH;
	return slot == 0 || slot == 10;
}

double apple_p2p_can_send_in(const struct apple_p2p_state *state, uint64_t now, int guard) {
	uint64_t next_aw = apple_p2p_sync_next_aw_us(now, &state->sync);
	uint64_t _guard = ieee80211_tu_to_usec(guard);
	uint64_t eaw = ieee80211_tu_to_usec(64);

	return (next_aw < _guard) ? -usec_to_sec(_guard - next_aw) : ((eaw - next_aw < _guard) ? usec_to_sec(
		(_guard - (eaw - next_aw))) : 0);
}

double apple_p2p_can_send_unicast_in(const struct apple_p2p_state *state, const struct apple_p2p_peer *peer, uint64_t now, int guard) {
	uint64_t next_aw = apple_p2p_sync_next_aw_us(now, &state->sync);
	uint64_t _guard = ieee80211_tu_to_usec(guard);
	uint64_t eaw = ieee80211_tu_to_usec(64);

	if (!apple_p2p_same_channel_as_peer(state, now, peer))
		return usec_to_sec(next_aw); /* try again in the next slot */

	if (next_aw < _guard) { /* we are at the end of slot */
		if (apple_p2p_same_channel_as_peer(state, now + eaw, peer)) {
			return 0; /* we are on the same channel in next slot, ignore guard */
		} else {
			return -usec_to_sec(_guard - next_aw);
		}
	} else if (eaw - next_aw < _guard) {
		if (apple_p2p_same_channel_as_peer(state, now - eaw, peer)) {
			return 0; /* we were on the same channel last slot, ignore guard */
		} else {
			return usec_to_sec(_guard - (eaw - next_aw));
		}
	} else {
		return 0; /* we are inside guard interval */
	}
}
