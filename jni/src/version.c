#include <stdio.h>

#include "version.h"

uint8_t apple_p2p_version(int major, int minor) {
	return ((uint8_t) (((major) << 4) & 0xf0) | ((minor) & 0x0f));
}

int apple_p2p_version_major(uint8_t version) {
	return (version >> 4) & 0xf;
}

int apple_p2p_version_minor(uint8_t version) {
	return (version & 0xf);
}

const char *apple_p2p_version_to_str(uint8_t version) {
	static char str[64];
	char *cur = str, *const end = str + sizeof(str);
	cur += snprintf(cur, end - cur, "%d", apple_p2p_version_major(version));
	cur += snprintf(cur, end - cur, ".");
	cur += snprintf(cur, end - cur, "%d", apple_p2p_version_minor(version));
	return str;
}

const char *apple_p2p_devclass_to_str(uint8_t devclass) {
	switch (devclass) {
		case APPLE_P2P_DEVCLASS_MACOS:
			return "macOS";
		case APPLE_P2P_DEVCLASS_IOS:
			return "iOS";
		case APPLE_P2P_DEVCLASS_TVOS:
			return "tvOS";
		default:
			return "Unknown";
	}
}
