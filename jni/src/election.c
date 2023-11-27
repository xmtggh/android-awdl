#include <string.h>
#include <stdio.h>

#include "election.h"
#include "peers.h"
#include "log.h"

static inline int compare(uint32_t a, uint32_t b) {
	return (a < b) ? -1 : (a > b);
}

int compare_ether_addr(const struct ether_addr *a, const struct ether_addr *b) {
	return memcmp(a, b, sizeof(struct ether_addr));
}

int apple_p2p_election_is_sync_master(const struct apple_p2p_election_state *state, const struct ether_addr *addr) {
	return !compare_ether_addr(&state->sync_addr, addr);
}

static void apple_p2p_election_reset_metric(struct apple_p2p_election_state *state) {
	state->self_counter = APPLE_P2P_ELECTION_COUNTER_INIT;
	state->self_metric = APPLE_P2P_ELECTION_METRIC_INIT;
}

static void apple_p2p_election_reset_self(struct apple_p2p_election_state *state) {
	state->height = 0;
	state->master_addr = state->self_addr;
	state->sync_addr = state->self_addr;
	state->master_metric = state->self_metric;
	state->master_counter = state->self_counter;
}

void apple_p2p_election_state_init(struct apple_p2p_election_state *state, const struct ether_addr *self) {
	state->master_addr = *self;
	state->sync_addr = *self;
	state->self_addr = *self;
	apple_p2p_election_reset_metric(state);
	apple_p2p_election_reset_self(state);
}

static int apple_p2p_election_compare_master(const struct apple_p2p_election_state *a, const struct apple_p2p_election_state *b) {
	int result = compare(a->master_counter, b->master_counter);
	if (!result)
		result = compare(a->master_metric, b->master_metric);
	return result;
}

void apple_p2p_election_run(struct apple_p2p_election_state *state, const struct apple_p2p_peer_state *peers) {
	struct apple_p2p_peer *peer;
	struct ether_addr old_top_master = state->master_addr;
	struct ether_addr old_sync_master = state->sync_addr;
	struct apple_p2p_election_state *master_state = state;
	apple_p2p_peers_it_t it = apple_p2p_peers_it_new(peers->peers);

	apple_p2p_election_reset_self(state);

	/* probably not fully correct */
	while (apple_p2p_peers_it_next(it, &peer) == PEERS_OK) {
		int cmp_metric;
		struct apple_p2p_election_state *peer_state = &peer->election;
		if (!peer->is_valid)
			continue; /* reject: not a valid peer */
		if (peer_state->height + 1 > APPLE_P2P_ELECTION_TREE_MAX_HEIGHT) {
			log_debug("Ignore peer %s because sync tree would get too large (%u, max %u)",
			          ether_ntoa(&peer_state->self_addr), peer_state->height + 1, APPLE_P2P_ELECTION_TREE_MAX_HEIGHT);
			continue; /* reject: tree would get too large if accepted as sync master */
		}
		if (apple_p2p_election_is_sync_master(peer_state, &state->self_addr))
			continue; /* reject: do not allow cycles in sync tree */
		cmp_metric = apple_p2p_election_compare_master(peer_state, master_state);
		if (cmp_metric < 0) {
			continue; /* reject: lower top master metric */
		} else if (cmp_metric == 0) {
			/* if metric is same, compare distance to master */
			if (peer_state->height > master_state->height) {
				continue; /* reject: do not increase length of sync tree */
			} else if (peer_state->height == master_state->height) {
				/* if height is same, need tie breaker */
				if (compare_ether_addr(&peer_state->self_addr, &master_state->self_addr) <= 0)
					continue; /* reject: peer has smaller address */
			}
		}
		/* accept: otherwise */
		master_state = peer_state;
	}

	if (state != master_state) { /* adopt new master */
		state->master_addr = master_state->master_addr;
		state->sync_addr = master_state->self_addr;
		state->master_metric = master_state->master_metric;
		state->master_counter = master_state->master_counter;
		state->height = master_state->height + 1;
	}

	if (compare_ether_addr(&old_top_master, &state->master_addr) ||
	    compare_ether_addr(&old_sync_master, &state->sync_addr)) {
		char tree[256];
		apple_p2p_election_tree_print(state, tree, sizeof(tree));
		log_debug("new election tree: %s", tree);
	}
}

int apple_p2p_election_tree_print(const struct apple_p2p_election_state *state, char *str, int len) {
	char *cur = str, *const end = str + len;
	cur += snprintf(cur, cur < end ? end - cur: 0, "%s", ether_ntoa(&state->self_addr));
	if (state->height > 0)
		cur += snprintf(cur, cur < end ? end - cur : 0, " -> %s", ether_ntoa(&state->sync_addr));
	if (state->height > 1) {
		cur += snprintf(cur, cur < end ? end - cur : 0, " ");
		for (uint32_t i = 1; i < state->height; i++)
			cur += snprintf(cur, cur < end ? end - cur : 0, "-"); /* one dash for every intermediate hop */
		cur += snprintf(cur, cur < end ? end - cur : 0, "> %s", ether_ntoa(&state->master_addr));
	}
	cur += snprintf(cur, cur < end ? end - cur : 0, " (met %u, ctr %u)", state->master_metric, state->master_counter);
	return cur - str;
}
