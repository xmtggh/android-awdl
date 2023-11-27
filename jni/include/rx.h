#ifndef __RX_H__
#define __RX_H__

#include "frame.h"
#include "version.h"
#include "state.h"

enum RX_RESULT {
	RX_IGNORE_PEER = 6,
	RX_IGNORE_RSSI = 5,
	RX_IGNORE_FAILED_CRC = 4,
	RX_IGNORE_NOPROMISC = 3,
	RX_IGNORE_FROM_SELF = 2,
	RX_IGNORE = 1,
	RX_OK = 0,
	RX_UNEXPECTED_FORMAT = -1,
	RX_UNEXPECTED_TYPE = -2,
	RX_UNEXPECTED_VALUE = -3,
	RX_TOO_SHORT = -3,
};

int apple_p2p_handle_sync_params_tlv(struct apple_p2p_peer *src, const struct buf *val, struct apple_p2p_state *state, uint64_t tsft);

int apple_p2p_handle_chanseq_tlv(struct apple_p2p_peer *src, const struct buf *val, struct apple_p2p_state *state);

int apple_p2p_handle_election_params_tlv(struct apple_p2p_peer *src, const struct buf *val, struct apple_p2p_state *state);

int apple_p2p_handle_election_params_v2_tlv(struct apple_p2p_peer *src, const struct buf *val,
                                       struct apple_p2p_state *state);

int apple_p2p_handle_arpa_tlv(struct apple_p2p_peer *src, const struct buf *val, struct apple_p2p_state *state);

int apple_p2p_handle_data_path_state_tlv(struct apple_p2p_peer *src, const struct buf *val, struct apple_p2p_state *state);

int apple_p2p_handle_version_tlv(struct apple_p2p_peer *src, const struct buf *val, struct apple_p2p_state *state);

int apple_p2p_handle_tlv(struct apple_p2p_peer *src, uint8_t type, const struct buf *val,
                    struct apple_p2p_state *state, uint64_t tsft);

int apple_p2p_parse_action_hdr(const struct buf *frame);

int apple_p2p_rx_action(const struct buf *frame, signed char rssi, uint64_t tsft,
                   const struct ether_addr *src, const struct ether_addr *dst, struct apple_p2p_state *state);

int llc_parse(const struct buf *frame, struct llc_hdr *llc);
int apple_p2p_valid_llc_header(const struct buf *frame);

int apple_p2p_rx_data(const struct buf *frame, struct buf ***out,
                 const struct ether_addr *src, const struct ether_addr *dst, struct apple_p2p_state *state);

int apple_p2p_rx_data_amsdu(const struct buf *frame, struct buf ***out,
                       const struct ether_addr *src, const struct ether_addr *dst, struct apple_p2p_state *state);

int apple_p2p_rx(const struct buf *frame, struct buf ***data_frames, struct apple_p2p_state *state);

#endif /* __RX_H__ */
