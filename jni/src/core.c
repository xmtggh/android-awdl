#include "core.h"
#include "netutils.h"

#include "log.h"
#include "wire.h"
#include "rx.h"
#include "tx.h"
#include "schedule.h"

#include <signal.h>

#ifdef __APPLE__
# define SIGSTATS SIGINFO
#else
# define SIGSTATS SIGUSR1
#endif

#define ETHER_LENGTH 14
#define ETHER_DST_OFFSET 0
#define ETHER_SRC_OFFSET 6
#define ETHER_ETHERTYPE_OFFSET 12

#define POLL_NEW_UNICAST 0x1
#define POLL_NEW_MULTICAST 0x2

static void dump_frame(const char *dump_file, const struct pcap_pkthdr *hdr, const uint8_t *buf) {
	if (dump_file) {
		/* Make sure file exists because 'pcap_dump_open_append' does NOT create file for you */
		fclose(fopen(dump_file, "a+"));
		pcap_t *p = pcap_open_dead(DLT_IEEE802_11_RADIO, 65535);
		pcap_dumper_t *dumper = pcap_dump_open_append(p, dump_file);
		pcap_dump((u_char *) dumper, hdr, buf);
		pcap_close(p);
		pcap_dump_close(dumper);
	}
}

static void ev_timer_rearm(struct ev_loop *loop, ev_timer *timer, double in) {
	ev_timer_stop(loop, timer);
	ev_timer_set(timer, in, 0.);
	ev_timer_start(loop, timer);
}

void wlan_device_ready(struct ev_loop *loop, ev_io *handle, int revents) {
    log_error("wlan_device_ready");
	struct daemon_state *state = handle->data;
	int cnt = pcap_dispatch(state->io.wlan_handle, 1, &apple_p2p_receive_frame, handle->data);
	if (cnt > 0)
		ev_feed_event(loop, handle, revents);
}

static int poll_host_device(struct daemon_state *state) {
	struct buf *buf = NULL;
	int result = 0;
	while (!state->next && !circular_buf_full(state->tx_queue_multicast)) {
		buf = buf_new_owned(ETHER_MAX_LEN);
		int len = buf_len(buf);
		if (host_recv(&state->io, (uint8_t *) buf_data(buf), &len) < 0) {
			goto wire_error;
		} else {
			bool is_multicast;
			struct ether_addr dst;
			buf_take(buf, buf_len(buf) - len);
			READ_ETHER_ADDR(buf, ETHER_DST_OFFSET, &dst);
			is_multicast = dst.ether_addr_octet[0] & 0x01;
			if (is_multicast) {
				circular_buf_put(state->tx_queue_multicast, buf);
				result |= POLL_NEW_MULTICAST;
			} else { /* unicast */
				state->next = buf;
				result |= POLL_NEW_UNICAST;
			}
		}
	}
	return result;
wire_error:
	if (buf)
		buf_free(buf);
	return result;
}

void host_device_ready(struct ev_loop *loop, ev_io *handle, int revents) {
	(void) loop;
	(void) revents; /* should always be EV_READ */
	struct daemon_state *state = handle->data;

	int poll_result = poll_host_device(state); /* fill TX queues */
	if (poll_result & POLL_NEW_MULTICAST)
		apple_p2p_send_multicast(loop, &state->ev_state.tx_mcast_timer, 0);
	if (poll_result & POLL_NEW_UNICAST)
		apple_p2p_send_unicast(loop, &state->ev_state.tx_timer, 0);
}

void apple_p2p_receive_frame(uint8_t *user, const struct pcap_pkthdr *hdr, const uint8_t *buf) {
    log_error("apple_p2p_receive_frame");
#define MAX_NUM_AMPDU 16 /* TODO lookup this constant from the standard */
	struct daemon_state *state = (void *) user;
	int result;
	const struct buf *frame = buf_new_const(buf, hdr->caplen);
	struct buf *data_arr[MAX_NUM_AMPDU];
	struct buf **data = &data_arr[0];
	result = apple_p2p_rx(frame, &data, &state->apple_p2p_state);
	if (result == RX_OK) {
		for (struct buf **data_start = &data_arr[0]; data_start < data; data_start++) {
			host_send(&state->io, buf_data(*data_start), buf_len(*data_start));
			buf_free(*data_start);
		}
	} else if (result < RX_OK) {
		log_warn("unhandled frame (%d)", result);
		dump_frame(state->dump, hdr, buf);
		state->apple_p2p_state.stats.rx_unknown++;
	}
	buf_free(frame);
}

int apple_p2p_send_data(const struct buf *buf, const struct io_state *io_state,
                   struct apple_p2p_state *apple_p2p_state, struct ieee80211_state *ieee80211_state) {
	uint8_t apple_p2p_data[65535];
	int apple_p2p_data_len;
	uint16_t ethertype;
	struct ether_addr src, dst;
	uint64_t now;
	uint16_t period, slot, tu;

	READ_BE16(buf, ETHER_ETHERTYPE_OFFSET, &ethertype);
	READ_ETHER_ADDR(buf, ETHER_DST_OFFSET, &dst);
	READ_ETHER_ADDR(buf, ETHER_SRC_OFFSET, &src);

	buf_strip(buf, ETHER_LENGTH);
	apple_p2p_data_len = apple_p2p_init_full_data_frame(apple_p2p_data, &src, &dst,
	                                          buf_data(buf), buf_len(buf),
	                                          apple_p2p_state, ieee80211_state);
	now = clock_time_us();
	period = apple_p2p_sync_current_eaw(now, &apple_p2p_state->sync) / APPLE_P2P_CHANSEQ_LENGTH;
	slot = apple_p2p_sync_current_eaw(now, &apple_p2p_state->sync) % APPLE_P2P_CHANSEQ_LENGTH;
	tu = apple_p2p_sync_next_aw_tu(now, &apple_p2p_state->sync);
	log_trace("Send data (len %d) to %s (%u.%u.%u)", apple_p2p_data_len,
	          ether_ntoa(&dst), period, slot, tu);
	apple_p2p_state->stats.tx_data++;
	if (wlan_send(io_state, apple_p2p_data, apple_p2p_data_len) < 0)
		return TX_FAIL;
	return TX_OK;

wire_error:
	return TX_FAIL;
}

void apple_p2p_send_action(struct daemon_state *state, enum apple_p2p_action_type type) {
	uint8_t buf[65535];
	int len;

	len = apple_p2p_init_full_action_frame(buf, &state->apple_p2p_state, &state->ieee80211_state, type);
	if (len < 0)
		return;
//	log_trace("send %s", apple_p2p_frame_as_str(type));
	wlan_send(&state->io, buf, len);

	state->apple_p2p_state.stats.tx_action++;
}

void apple_p2p_send_psf(struct ev_loop *loop, ev_timer *handle, int revents) {
	(void) loop;
	(void) revents;
	apple_p2p_send_action(handle->data, APPLE_P2P_ACTION_PSF);
}

void apple_p2p_send_mif(struct ev_loop *loop, ev_timer *timer, int revents) {
	(void) revents;
	struct daemon_state *state = timer->data;
	struct apple_p2p_state *apple_p2p_state = &state->apple_p2p_state;
	uint64_t now, next_aw, eaw_len;

	now = clock_time_us();
	next_aw = apple_p2p_sync_next_aw_us(now, &apple_p2p_state->sync);
	eaw_len = apple_p2p_state->sync.presence_mode * apple_p2p_state->sync.aw_period;

	/* Schedule MIF in middle of sequence (if non-zero) */
	if (apple_p2p_chan_num(apple_p2p_state->channel.current, apple_p2p_state->channel.enc) > 0)
		apple_p2p_send_action(state, APPLE_P2P_ACTION_MIF);

	/* schedule next in the middle of EAW */
	ev_timer_rearm(loop, timer, usec_to_sec(next_aw + ieee80211_tu_to_usec(eaw_len / 2)));
}

void apple_p2p_send_unicast(struct ev_loop *loop, ev_timer *timer, int revents) {
	(void) revents;
	struct daemon_state *state = timer->data;
	struct apple_p2p_state *apple_p2p_state = &state->apple_p2p_state;
	uint64_t now = clock_time_us();
	double in = 0;

	if (state->next) { /* we have something to send */
		struct apple_p2p_peer *peer;
		struct ether_addr dst;
		read_ether_addr(state->next, ETHER_DST_OFFSET, &dst);
		if (compare_ether_addr(&dst, &apple_p2p_state->self_address) == 0) {
			/* send back to self */
			host_send(&state->io, buf_data(state->next), buf_len(state->next));
			buf_free(state->next);
			state->next = NULL;
		} else if (apple_p2p_peer_get(apple_p2p_state->peers.peers, &dst, &peer) < 0) {
			log_debug("Drop frame to non-peer %s", ether_ntoa(&dst));
			buf_free(state->next);
			state->next = NULL;
		} else {
			in = apple_p2p_can_send_unicast_in(apple_p2p_state, peer, now, APPLE_P2P_UNICAST_GUARD_TU);
			if (in == 0) { /* send now */
				apple_p2p_send_data(state->next, &state->io, &state->apple_p2p_state, &state->ieee80211_state);
				buf_free(state->next);
				state->next = NULL;
				state->apple_p2p_state.stats.tx_data_unicast++;
			} else { /* try later */
				if (in < 0) /* we are at the end of slot but within guard */
					in = -in + usec_to_sec(ieee80211_tu_to_usec(APPLE_P2P_UNICAST_GUARD_TU));
			}
		}
	}

	/* rearm if more unicast frames available */
	if (state->next) {
		log_trace("apple_p2p_send_unicast: retry in %lu TU", ieee80211_usec_to_tu(sec_to_usec(in)));
		ev_timer_rearm(loop, timer, in);
	} else {
		/* poll for more frames to keep queue full */
		ev_feed_event(loop, &state->ev_state.read_host, 0);
	}
}

void apple_p2p_send_multicast(struct ev_loop *loop, ev_timer *timer, int revents) {
	(void) revents;
	struct daemon_state *state = timer->data;
	struct apple_p2p_state *apple_p2p_state = &state->apple_p2p_state;
	uint64_t now = clock_time_us();
	double in = 0;

	if (!circular_buf_empty(state->tx_queue_multicast)) { /* we have something to send */
		in = apple_p2p_can_send_in(apple_p2p_state, now, APPLE_P2P_MULTICAST_GUARD_TU);
		if (apple_p2p_is_multicast_eaw(apple_p2p_state, now) && (in == 0)) { /* we can send now */
			void *next;
			circular_buf_get(state->tx_queue_multicast, &next, 0);
			apple_p2p_send_data((struct buf *) next, &state->io, &state->apple_p2p_state, &state->ieee80211_state);
			buf_free(next);
			state->apple_p2p_state.stats.tx_data_multicast++;
		} else { /* try later */
			if (in == 0) /* try again next EAW */
				in = usec_to_sec(ieee80211_tu_to_usec(64));
			else if (in < 0) /* we are at the end of slot but within guard */
				in = -in + usec_to_sec(ieee80211_tu_to_usec(APPLE_P2P_MULTICAST_GUARD_TU));
		}
	}

	/* rearm if more multicast frames available */
	if (!circular_buf_empty(state->tx_queue_multicast)) {
		//log_trace("apple_p2p_send_multicast: retry in %lu TU", ieee80211_usec_to_tu(sec_to_usec(in)));
		ev_timer_rearm(loop, timer, in);
	} else {
		/* poll for more frames to keep queue full */
		ev_feed_event(loop, &state->ev_state.read_host, 0);
	}
}

void apple_p2p_switch_channel(struct ev_loop *loop, ev_timer *timer, int revents) {
	(void) revents;
	uint64_t now, next_aw;
	int slot;
	struct apple_p2p_chan chan_new;
	int chan_num_new, chan_num_old;
	struct daemon_state *state = timer->data;
	struct apple_p2p_state *apple_p2p_state = &state->apple_p2p_state;

	chan_num_old = apple_p2p_chan_num(apple_p2p_state->channel.current, apple_p2p_state->channel.enc);

	now = clock_time_us();
	slot = apple_p2p_sync_current_eaw(now, &apple_p2p_state->sync) % APPLE_P2P_CHANSEQ_LENGTH;
	chan_new = apple_p2p_state->channel.sequence[slot];
	chan_num_new = apple_p2p_chan_num(apple_p2p_state->channel.sequence[slot], apple_p2p_state->channel.enc);

	if (chan_num_new && (chan_num_new != chan_num_old)) {
		log_debug("switch channel to %d (slot %d)", chan_num_new, slot);
		if (!state->io.wlan_is_file) {
			bool is_available;
			is_channel_available(state->io.wlan_ifindex, chan_num_new, &is_available);
			set_channel(state->io.wlan_ifindex, chan_num_new);
		}
		apple_p2p_state->channel.current = chan_new;
	}

	next_aw = apple_p2p_sync_next_aw_us(now, &apple_p2p_state->sync);

	ev_timer_rearm(loop, timer, usec_to_sec(next_aw));
}

static void apple_p2p_neighbor_add(struct apple_p2p_peer *p, void *_io_state) {
	struct io_state *io_state = _io_state;
	neighbor_add_rfc4291(io_state->host_ifindex, &p->addr);
}

static void apple_p2p_neighbor_remove(struct apple_p2p_peer *p, void *_io_state) {
	struct io_state *io_state = _io_state;
	neighbor_remove_rfc4291(io_state->host_ifindex, &p->addr);
}

void apple_p2p_clean_peers(struct ev_loop *loop, ev_timer *timer, int revents) {
	(void) loop;
	(void) revents; /* should always be EV_TIMER */
	uint64_t cutoff_time;
	struct daemon_state *state;

	state = (struct daemon_state *) timer->data;
	cutoff_time = clock_time_us() - state->apple_p2p_state.peers.timeout;

	apple_p2p_peers_remove(state->apple_p2p_state.peers.peers, cutoff_time,
	                  state->apple_p2p_state.peer_remove_cb, state->apple_p2p_state.peer_remove_cb_data);

	/* TODO for now run election immediately after clean up; might consider seperate timer for this */
	apple_p2p_election_run(&state->apple_p2p_state.election, &state->apple_p2p_state.peers);

	ev_timer_again(loop, timer);
}

void apple_p2p_print_stats(struct ev_loop *loop, ev_signal *handle, int revents) {
	(void) loop;
	(void) revents; /* should always be EV_TIMER */
	struct apple_p2p_stats *stats = &((struct daemon_state *) handle->data)->apple_p2p_state.stats;

	log_info("STATISTICS");
	log_info(" TX action %llu, data %llu, unicast %llu, multicast %llu",
	         stats->tx_action, stats->tx_data, stats->tx_data_unicast, stats->tx_data_multicast);
	log_info(" RX action %llu, data %llu, unknown %llu",
	         stats->rx_action, stats->rx_data, stats->rx_unknown);
}

int apple_p2p_init(struct daemon_state *state, const char *wlan, const char *host, struct apple_p2p_chan chan, const char *dump) {
	int err;
	char hostname[HOST_NAME_LENGTH_MAX + 1];
	log_warn("apple_p2p_init 11111");
	err = netutils_init();
	if (err < 0)
		return err;
	log_warn("apple_p2p_init 22222");
	err = io_state_init(&state->io, wlan, host, &APPLE_P2P_BSSID);
	if (err < 0)
		return err;
	log_warn("apple_p2p_init 333333");
	err = get_hostname(hostname, sizeof(hostname));
	if (err < 0)
		return err;
    log_warn("apple_p2p_init 4444");
	apple_p2p_init_state(&state->apple_p2p_state, hostname, &state->io.if_ether_addr, chan, clock_time_us());
	state->apple_p2p_state.peer_cb = apple_p2p_neighbor_add;
	state->apple_p2p_state.peer_cb_data = (void *) &state->io;
	state->apple_p2p_state.peer_remove_cb = apple_p2p_neighbor_remove;
	state->apple_p2p_state.peer_remove_cb_data = (void *) &state->io;
	ieee80211_init_state(&state->ieee80211_state);
    log_warn("apple_p2p_init 5555");
	state->next = NULL;
	state->tx_queue_multicast = circular_buf_init(16);
	state->dump = dump;
    log_warn("apple_p2p_init 66666");
	return 0;
}

void apple_p2p_free(struct daemon_state *state) {
	circular_buf_free(state->tx_queue_multicast);
	io_state_free(&state->io);
	netutils_cleanup();
}

void apple_p2p_schedule(struct ev_loop *loop, struct daemon_state *state) {
    log_error("apple_p2p_schedule 111");
	state->ev_state.loop = loop;

	/* Timer for channel switching */
	state->ev_state.chan_timer.data = (void *) state;
	ev_timer_init(&state->ev_state.chan_timer, apple_p2p_switch_channel, 0, 0);
	ev_timer_start(loop, &state->ev_state.chan_timer);
    log_error("apple_p2p_schedule 22222");
	/* Timer for peer table cleanup */
	state->ev_state.peer_timer.data = (void *) state;
	ev_timer_init(&state->ev_state.peer_timer, apple_p2p_clean_peers, 0, usec_to_sec(state->apple_p2p_state.peers.clean_interval));
	ev_timer_again(loop, &state->ev_state.peer_timer);
    log_error("apple_p2p_schedule 33333");
	/* Trigger frame reception from WLAN device */
	state->ev_state.read_wlan.data = (void *) state;
	ev_io_init(&state->ev_state.read_wlan, wlan_device_ready, state->io.wlan_fd, EV_READ);
	ev_io_start(loop, &state->ev_state.read_wlan);
    log_error("apple_p2p_schedule 444444");
	/* Trigger frame reception from host device */
	state->ev_state.read_host.data = (void *) state;
	ev_io_init(&state->ev_state.read_host, host_device_ready, state->io.host_fd, EV_READ);
	ev_io_start(loop, &state->ev_state.read_host);
    log_error("apple_p2p_schedule 55555");
	/* Timer for PSFs */
	state->ev_state.psf_timer.data = (void *) state;
	ev_timer_init(&state->ev_state.psf_timer, apple_p2p_send_psf,
	              usec_to_sec(ieee80211_tu_to_usec(state->apple_p2p_state.psf_interval)),
	              usec_to_sec(ieee80211_tu_to_usec(state->apple_p2p_state.psf_interval)));
	ev_timer_start(loop, &state->ev_state.psf_timer);
    log_error("apple_p2p_schedule 666666");
	/* Timer for MIFs */
	state->ev_state.mif_timer.data = (void *) state;
	ev_timer_init(&state->ev_state.mif_timer, apple_p2p_send_mif, 0, 0);
	ev_timer_start(loop, &state->ev_state.mif_timer);
    log_error("apple_p2p_schedule 77777");
	/* Timer for unicast packets */
	state->ev_state.tx_timer.data = (void *) state;
	ev_timer_init(&state->ev_state.tx_timer, apple_p2p_send_unicast, 0, 0);
	ev_timer_start(loop, &state->ev_state.tx_timer);

    log_error("apple_p2p_schedule 88888");
	/* Timer for multicast packets */
	state->ev_state.tx_mcast_timer.data = (void *) state;
	ev_timer_init(&state->ev_state.tx_mcast_timer, apple_p2p_send_multicast, 0, 0);
	ev_timer_start(loop, &state->ev_state.tx_mcast_timer);
    log_error("apple_p2p_schedule 9999");
	/* Register signal to print statistics */
	state->ev_state.stats.data = (void *) state;
	ev_signal_init(&state->ev_state.stats, apple_p2p_print_stats, SIGSTATS);
	ev_signal_start(loop, &state->ev_state.stats);
}
