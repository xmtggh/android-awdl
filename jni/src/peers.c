#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "peers.h"
#include "hashmap.h"
#include "log.h"

#define PEERS_DEFAULT_TIMEOUT        2000000 /* in ms */
#define PEERS_DEFAULT_CLEAN_INTERVAL 1000000 /* in ms */

void apple_p2p_peer_state_init(struct apple_p2p_peer_state *state) {
	state->peers = apple_p2p_peers_init();
	state->timeout = PEERS_DEFAULT_TIMEOUT;
	state->clean_interval = PEERS_DEFAULT_CLEAN_INTERVAL;
}

apple_p2p_peers_t apple_p2p_peers_init() {
	return (apple_p2p_peers_t) hashmap_new(sizeof(struct ether_addr));
}

void apple_p2p_peers_free(apple_p2p_peers_t peers) {
	struct apple_p2p_peer *peer;
	map_t map = (map_t) peers;

	/* Remove all allocated peers */
	map_it_t it = hashmap_it_new(map);
	while (hashmap_it_next(it, NULL, (any_t *) &peer) == MAP_OK) {
		hashmap_it_remove(it);
		free(peer);
	}
	hashmap_it_free(it);

	/* Remove hashmap itself */
	hashmap_free(peers);
}

int apple_p2p_peers_length(apple_p2p_peers_t peers) {
	map_t map = (map_t) peers;
	return hashmap_length(map);
}

static int apple_p2p_peer_is_valid(const struct apple_p2p_peer *peer) {
	return peer->sent_mif && peer->devclass && peer->version;
}

struct apple_p2p_peer *apple_p2p_peer_new(const struct ether_addr *addr) {
	struct apple_p2p_peer *peer = (struct apple_p2p_peer *) malloc(sizeof(struct apple_p2p_peer));
	*(struct ether_addr *) &peer->addr = *addr;
	peer->last_update = 0;
	apple_p2p_election_state_init(&peer->election, addr);
	apple_p2p_chanseq_init_static(peer->sequence, &CHAN_NULL);
	peer->sync_offset = 0;
	peer->devclass = 0;
	peer->version = 0;
	peer->supports_v2 = 0;
	peer->sent_mif = 0;
	strcpy(peer->name, "");
	strcpy(peer->country_code, "NA");
	peer->is_valid = 0;
	return peer;
}

enum peers_status
apple_p2p_peer_add(apple_p2p_peers_t peers, const struct ether_addr *_addr, uint64_t now, apple_p2p_peer_cb cb, void *arg) {
	int status, result;
	map_t map = (map_t) peers;
	mkey_t addr = (mkey_t) _addr;
	struct apple_p2p_peer *peer;
	status = hashmap_get(map, addr, (any_t *) &peer, 0 /* do not remove */);
	if (status == MAP_MISSING)
		peer = apple_p2p_peer_new(_addr); /* create new entry */

	/* update */
	peer->last_update = now;

	if (status == MAP_OK) {
		result = PEERS_UPDATE;
		goto out;
	}

	status = hashmap_put(map, (mkey_t) &peer->addr, peer);
	if (status != MAP_OK) {
		free(peer);
		return PEERS_INTERNAL;
	}
	result = PEERS_OK;
out:
	if (!peer->is_valid && apple_p2p_peer_is_valid(peer)) {
		/* peer has turned valid */
		peer->is_valid = 1;
		log_info("add peer %s (%s)", ether_ntoa(&peer->addr), peer->name);
		if (cb)
			cb(peer, arg);
	}
	return result;
}

enum peers_status apple_p2p_peer_remove(apple_p2p_peers_t peers, const struct ether_addr *_addr, apple_p2p_peer_cb cb, void *arg) {
	int status;
	map_t map = (map_t) peers;
	mkey_t addr = (mkey_t) _addr;
	struct apple_p2p_peer *peer;
	status = hashmap_get(map, addr, (any_t *) &peer, 1 /* remove */);
	if (status == MAP_MISSING)
		return PEERS_MISSING;
	if (peer->is_valid) {
		log_info("remove peer %s (%s)", ether_ntoa(&peer->addr), peer->name);
		if (cb)
			cb(peer, arg);
	}
	free(peer);
	return PEERS_OK;
}

enum peers_status apple_p2p_peer_get(apple_p2p_peers_t peers, const struct ether_addr *_addr, struct apple_p2p_peer **peer) {
	int status;
	map_t map = (map_t) peers;
	mkey_t addr = (mkey_t) _addr;
	status = hashmap_get(map, addr, (any_t *) peer, 0 /* keep */);
	if (status == MAP_MISSING)
		return PEERS_MISSING;
	return PEERS_OK;
}

int apple_p2p_peer_print(const struct apple_p2p_peer *peer, char *str, int len) {
	char *cur = str, *const end = str + len;
	if (strlen(peer->name))
		cur += snprintf(cur, cur < end ? end - cur : 0, "%s: ", peer->name);
	else
		cur += snprintf(cur, cur < end ? end - cur : 0, "<UNNAMED>: ");
    cur += apple_p2p_election_tree_print(&peer->election, cur, end - cur);
	return cur - str;
}

int apple_p2p_peers_print(apple_p2p_peers_t peers, char *str, int len) {
	char *cur = str, *const end = str + len;
	map_t map = (map_t) peers;
	map_it_t it = hashmap_it_new(map);
	struct apple_p2p_peer *peer;

	while (hashmap_it_next(it, NULL, (any_t *) &peer) == MAP_OK) {
        cur += apple_p2p_peer_print(peer, cur, end - cur);
        cur += snprintf(cur, cur < end ? end - cur : 0, "\n");
    }

	hashmap_it_free(it);
	return cur - str;
}

void apple_p2p_peers_remove(apple_p2p_peers_t peers, uint64_t before, apple_p2p_peer_cb cb, void *arg) {
	map_t map = (map_t) peers;
	map_it_t it = hashmap_it_new(map);
	struct apple_p2p_peer *peer;

	while (hashmap_it_next(it, NULL, (any_t *) &peer) == MAP_OK) {
		if (peer->last_update < before) {
			if (peer->is_valid) {
				log_info("remove peer %s (%s)", ether_ntoa(&peer->addr), peer->name);
				if (cb)
					cb(peer, arg);
			}
			hashmap_it_remove(it);
			free(peer);
		}
	}
	hashmap_it_free(it);
}

apple_p2p_peers_it_t apple_p2p_peers_it_new(apple_p2p_peers_t in) {
	return (apple_p2p_peers_it_t) hashmap_it_new((map_t) in);
}

enum peers_status apple_p2p_peers_it_next(apple_p2p_peers_it_t it, struct apple_p2p_peer **peer) {
	int s = hashmap_it_next((map_it_t) it, 0, (any_t *) peer);
	if (s == MAP_OK)
		return PEERS_OK;
	else
		return PEERS_MISSING;
}

enum peers_status apple_p2p_peers_it_remove(apple_p2p_peers_it_t it) {
	int s = hashmap_it_remove((map_it_t) it);
	if (s == MAP_OK)
		return PEERS_OK;
	else
		return PEERS_MISSING;
}

void apple_p2p_peers_it_free(apple_p2p_peers_it_t it) {
	hashmap_it_free((map_it_t) it);
}
