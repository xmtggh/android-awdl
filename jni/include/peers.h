#ifndef __PEERS_H__
#define __PEERS_H__

#include <stdint.h>
#include <net/ethernet.h>

#include "election.h"
#include "channel.h"

#define HOST_NAME_LENGTH_MAX 64

enum peers_status {
	PEERS_UPDATE = 1, /* Peer updated */
	PEERS_OK = 0, /* New peer added */
	PEERS_MISSING = -1, /* Peer does not exist */
	PEERS_INTERNAL = -2, /* Internal error */
};

struct apple_p2p_peer {
	const struct ether_addr addr;
	uint64_t last_update;
	struct apple_p2p_election_state election;
	struct apple_p2p_chan sequence[APPLE_P2P_CHANSEQ_LENGTH];
	uint64_t sync_offset;
	char name[HOST_NAME_LENGTH_MAX + 1]; /* space for trailing zero */
	char country_code[2 + 1];
	struct ether_addr infra_addr;
	uint8_t version;
	uint8_t devclass;
	uint8_t supports_v2 : 1;
	uint8_t sent_mif : 1;
	uint8_t is_valid : 1;
};

typedef void (*apple_p2p_peer_cb)(struct apple_p2p_peer *, void *arg);

typedef void *apple_p2p_peers_t;

struct apple_p2p_peer_state {
	apple_p2p_peers_t peers;
	uint64_t timeout;
	uint64_t clean_interval;
};

void apple_p2p_peer_state_init(struct apple_p2p_peer_state *state);

apple_p2p_peers_t apple_p2p_peers_init();

void apple_p2p_peers_free(apple_p2p_peers_t peers);

int apple_p2p_peers_length(apple_p2p_peers_t peers);

enum peers_status
apple_p2p_peer_add(apple_p2p_peers_t peers, const struct ether_addr *addr, uint64_t now, apple_p2p_peer_cb cb, void *arg);

enum peers_status apple_p2p_peer_remove(apple_p2p_peers_t peers, const struct ether_addr *addr, apple_p2p_peer_cb cb, void *arg);

enum peers_status apple_p2p_peer_get(apple_p2p_peers_t peers, const struct ether_addr *addr, struct apple_p2p_peer **peer);

int apple_p2p_peer_print(const struct apple_p2p_peer *peer, char *str, int len);

/**
 * Apply callback to and then remove all peers matching a filter
 * @param peers the apple_p2p_peers_t instance
 * @param before remove all entries with an last_update timestamp {@code before}
 * @param cb the callback function, can be NULL
 * @param arg will be passed to callback function
 */
void apple_p2p_peers_remove(apple_p2p_peers_t peers, uint64_t before, apple_p2p_peer_cb cb, void *arg);

int apple_p2p_peers_print(apple_p2p_peers_t peers, char *str, int len);

/* Iterator functions */
typedef void *apple_p2p_peers_it_t;

apple_p2p_peers_it_t apple_p2p_peers_it_new(apple_p2p_peers_t in);

enum peers_status apple_p2p_peers_it_next(apple_p2p_peers_it_t it, struct apple_p2p_peer **peer);

enum peers_status apple_p2p_peers_it_remove(apple_p2p_peers_it_t it);

void apple_p2p_peers_it_free(apple_p2p_peers_it_t it);

#endif /* __PEERS_H__ */
