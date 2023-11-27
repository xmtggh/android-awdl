#ifndef __VERSION_H__
#define __VERSION_H__

#include <stdint.h>

uint8_t apple_p2p_version(int major, int minor);

int apple_p2p_version_major(uint8_t version);

int apple_p2p_version_minor(uint8_t version);

const char *apple_p2p_version_to_str(uint8_t);

enum apple_p2p_devclass {
	APPLE_P2P_DEVCLASS_MACOS = 1,
	APPLE_P2P_DEVCLASS_IOS = 2,
	APPLE_P2P_DEVCLASS_TVOS = 8,
};

const char *apple_p2p_devclass_to_str(uint8_t devclass);

#endif /* __VERSION_H__ */
