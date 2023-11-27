#ifndef __SCHEDULE_H__
#define __SCHEDULE_H__

#include <stdbool.h>

#include "state.h"
#include "peers.h"

#define APPLE_P2P_UNICAST_GUARD_TU 3
#define APPLE_P2P_MULTICAST_GUARD_TU 16

double usec_to_sec(uint64_t usec);

uint64_t sec_to_usec(double sec);

bool apple_p2p_same_channel_as_peer(const struct apple_p2p_state *state, uint64_t now, const struct apple_p2p_peer *peer);

int apple_p2p_is_multicast_eaw(const struct apple_p2p_state *state, uint64_t now);

double apple_p2p_can_send_in(const struct apple_p2p_state *state, uint64_t now, int guard);

double apple_p2p_can_send_unicast_in(const struct apple_p2p_state *state, const struct apple_p2p_peer *peer, uint64_t now, int guard);

#endif /* __SCHEDULE_H__ */
