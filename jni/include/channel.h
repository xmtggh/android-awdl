#ifndef __CHANNEL_H__
#define __CHANNEL_H__

#include <stdint.h>

#define APPLE_P2P_CHANSEQ_LENGTH 16

enum apple_p2p_chan_encoding {
	APPLE_P2P_CHAN_ENC_SIMPLE = 0,
	APPLE_P2P_CHAN_ENC_LEGACY = 1,
	APPLE_P2P_CHAN_ENC_OPCLASS = 3,
};

struct apple_p2p_chan {
	union {
		uint8_t val[2];
		struct {
			uint8_t chan_num;
		} simple;
		struct {
			uint8_t flags;
			uint8_t chan_num;
		} legacy;
		struct {
			uint8_t chan_num;
			uint8_t opclass;
		} opclass;
	};
};

#define CHAN_NULL (struct apple_p2p_chan) { { { 0, 0x00 } } }
#define CHAN_OPCLASS_6 (struct apple_p2p_chan) { { { 6, 0x51 } } }
#define CHAN_OPCLASS_44 (struct apple_p2p_chan) { { { 44, 0x80 } } }
#define CHAN_OPCLASS_149 (struct apple_p2p_chan) { { { 149, 0x80 } } }

uint8_t apple_p2p_chan_num(struct apple_p2p_chan chan, enum apple_p2p_chan_encoding);

int apple_p2p_chan_encoding_size(enum apple_p2p_chan_encoding);

struct apple_p2p_channel_state {
	enum apple_p2p_chan_encoding enc;
	struct apple_p2p_chan sequence[APPLE_P2P_CHANSEQ_LENGTH];
	struct apple_p2p_chan master;
	struct apple_p2p_chan current;
};

void apple_p2p_chanseq_init(struct apple_p2p_chan *seq);

void apple_p2p_chanseq_init_idle(struct apple_p2p_chan *seq);

void apple_p2p_chanseq_init_static(struct apple_p2p_chan *seq, const struct apple_p2p_chan *chan);

#endif /* __CHANNEL_H__ */
