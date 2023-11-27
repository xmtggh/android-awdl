#ifndef __ELECTION_H__
#define __ELECTION_H__

#include "ieee80211.h"

#define APPLE_P2P_ELECTION_TREE_MAX_HEIGHT 10 /* arbitrary limit */
#define APPLE_P2P_ELECTION_METRIC_INIT 60
#define APPLE_P2P_ELECTION_COUNTER_INIT 0

struct apple_p2p_peer_state;

struct apple_p2p_election_state {
	struct ether_addr master_addr;
	struct ether_addr sync_addr;
	struct ether_addr self_addr;
	uint32_t height;
	uint32_t master_metric;
	uint32_t self_metric;
	uint32_t master_counter;
	uint32_t self_counter;
};

int apple_p2p_election_is_sync_master(const struct apple_p2p_election_state *state, const struct ether_addr *addr);

void apple_p2p_election_state_init(struct apple_p2p_election_state *state, const struct ether_addr *self);

void apple_p2p_election_run(struct apple_p2p_election_state *state, const struct apple_p2p_peer_state *peers);

int apple_p2p_election_tree_print(const struct apple_p2p_election_state *state, char *str, int len);

/* Util functions */
int compare_ether_addr(const struct ether_addr *a, const struct ether_addr *b);

#endif /* __ELECTION_H__ */
