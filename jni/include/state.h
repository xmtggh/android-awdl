#ifndef __STATE_H__
#define __STATE_H__

#include <stdint.h>
#include <stdbool.h>

#include "wire.h"
#include "frame.h"
#include "peers.h"
#include "sync.h"
#include "channel.h"

#define RSSI_THRESHOLD_DEFAULT -65
#define RSSI_GRACE_DEFAULT      -5

struct apple_p2p_state; /* forward declaration for tlv_cb */

typedef void (*apple_p2p_tlv_cb)(struct apple_p2p_peer *, uint8_t, const struct buf *, struct apple_p2p_state *, void *);

struct apple_p2p_stats {
	uint64_t tx_action;
	uint64_t tx_data;
	uint64_t tx_data_unicast;
	uint64_t tx_data_multicast;
	uint64_t rx_action;
	uint64_t rx_data;
	uint64_t rx_unknown;
};

/* Complete node state */
struct apple_p2p_state {
	struct ether_addr self_address;
	char name[HOST_NAME_LENGTH_MAX + 1];

	uint8_t version;
	uint8_t dev_class;

	/* sequence number for data frames */
	uint16_t sequence_number;
	/* PSF interval (in TU) */
	uint16_t psf_interval;
	/* destination address for action frames */
	struct ether_addr dst;

	/* Allows to hook TLV reception */
	apple_p2p_tlv_cb tlv_cb;
	void *tlv_cb_data;

	/* Allows to hook adding of new neighbor */
	apple_p2p_peer_cb peer_cb;
	void *peer_cb_data;

	/* Allows to hook removing of new neighbor */
	apple_p2p_peer_cb peer_remove_cb;
	void *peer_remove_cb_data;

	int filter_rssi; /* whether or not to filter (default: true) */
	signed char rssi_threshold; /* peers exceeding this threshold are discovered */
	signed char rssi_grace; /* once discovered accept lower RSSI */

	struct apple_p2p_election_state election;
	struct apple_p2p_sync_state sync;
	struct apple_p2p_channel_state channel;
	struct apple_p2p_peer_state peers;
	struct apple_p2p_stats stats;
};

void apple_p2p_stats_init(struct apple_p2p_stats *stats);

void apple_p2p_init_state(struct apple_p2p_state *state, const char *hostname, const struct ether_addr *self,
                     struct apple_p2p_chan chan, uint64_t now);

uint16_t apple_p2p_state_next_sequence_number(struct apple_p2p_state *);

struct ieee80211_state {
	/* IEEE 802.11 sequence number */
	uint16_t sequence_number;
	/* whether we need to add an fcs */
	bool fcs;
};

void ieee80211_init_state(struct ieee80211_state *);

unsigned int ieee80211_state_next_sequence_number(struct ieee80211_state *);

uint64_t clock_time_us();

#endif /* __STATE_H__ */
