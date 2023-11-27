#ifndef __CORE_H__
#define __CORE_H__

#include <stdbool.h>
#include <ev.h>

#include "state.h"
#include "circular_buffer.h"
#include "io.h"

struct ev_state {
	struct ev_loop *loop;
	ev_timer mif_timer, psf_timer, tx_timer, tx_mcast_timer, chan_timer, peer_timer;
	ev_io read_wlan, read_host;
	ev_signal stats;
};

struct daemon_state {
	struct io_state io;
	struct apple_p2p_state apple_p2p_state;
	struct ieee80211_state ieee80211_state;
	struct ev_state ev_state;
	struct buf *next;
	cbuf_handle_t tx_queue_multicast;
	const char *dump;
};

int apple_p2p_init(struct daemon_state *state, const char *wlan, const char *host, struct apple_p2p_chan chan, const char *dump);

void apple_p2p_free(struct daemon_state *state);

void apple_p2p_schedule(struct ev_loop *loop, struct daemon_state *state);

void wlan_device_ready(struct ev_loop *loop, ev_io *handle, int revents);

void host_device_ready(struct ev_loop *loop, ev_io *handle, int revents);

void apple_p2p_receive_frame(uint8_t *user, const struct pcap_pkthdr *hdr, const uint8_t *buf);

void apple_p2p_send_action(struct daemon_state *state, enum apple_p2p_action_type type);

void apple_p2p_send_psf(struct ev_loop *loop, ev_timer *handle, int revents);

void apple_p2p_send_mif(struct ev_loop *loop, ev_timer *handle, int revents);

void apple_p2p_send_unicast(struct ev_loop *loop, ev_timer *timer, int revents);

void apple_p2p_send_multicast(struct ev_loop *loop, ev_timer *timer, int revents);

int apple_p2p_send_data(const struct buf *buf, const struct io_state *io_state,
                   struct apple_p2p_state *apple_p2p_state, struct ieee80211_state *ieee80211_state);

void apple_p2p_switch_channel(struct ev_loop *loop, ev_timer *handle, int revents);

void apple_p2p_clean_peers(struct ev_loop *loop, ev_timer *timer, int revents);

void apple_p2p_print_stats(struct ev_loop *loop, ev_signal *handle, int revents);

#endif /* __CORE_H__ */
