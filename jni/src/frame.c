#include "frame.h"

const char *apple_p2p_frame_as_str(uint8_t type) {
	switch (type) {
		case APPLE_P2P_ACTION_PSF:
			return "PSF";
		case APPLE_P2P_ACTION_MIF:
			return "MIF";
		default:
			return "Unknown";
	}
}

const char *apple_p2p_tlv_as_str(uint8_t type) {
	switch (type) {
		case APPLE_P2P_SSTH_REQUEST_TLV:
			return "SSTH Request";
		case APPLE_P2P_SERVICE_REQUEST_TLV:
			return "Service Request";
		case APPLE_P2P_SERVICE_RESPONSE_TLV:
			return "Service Response";
		case APPLE_P2P_SYNCHRONIZATON_PARAMETERS_TLV:
			return "Synchronization Parameters";
		case APPLE_P2P_ELECTION_PARAMETERS_TLV:
			return "Election Parameters";
		case APPLE_P2P_SERVICE_PARAMETERS_TLV:
			return "Service Parameters";
		case APPLE_P2P_ENHANCED_DATA_RATE_CAPABILITIES_TLV:
			return "HT Capabilities";
		case APPLE_P2P_ENHANCED_DATA_RATE_OPERATION_TLV:
			return "HT Operation";
		case APPLE_P2P_INFRA_TLV:
			return "Infra";
		case APPLE_P2P_INVITE_TLV:
			return "Invite";
		case APPLE_P2P_DBG_STRING_TLV:
			return "Debug String";
		case APPLE_P2P_DATA_PATH_STATE_TLV:
			return "Data Path State";
		case APPLE_P2P_ENCAPSULATED_IP_TLV:
			return "Encapsulated IP";
		case APPLE_P2P_DATAPATH_DEBUG_PACKET_LIVE_TLV:
			return "Datapath Debug Packet Live";
		case APPLE_P2P_DATAPATH_DEBUG_AF_LIVE_TLV:
			return "Datapath Debug AF Live";
		case APPLE_P2P_ARPA_TLV:
			return "Arpa";
		case APPLE_P2P_IEEE80211_CNTNR_TLV:
			return "VHT Capabilities";
		case APPLE_P2P_CHAN_SEQ_TLV:
			return "Channel Sequence";
		case APPLE_P2P_SYNCTREE_TLV:
			return "Synchronization Tree";
		case APPLE_P2P_VERSION_TLV:
			return "Version";
		case APPLE_P2P_BLOOM_FILTER_TLV:
			return "Bloom Filter";
		case APPLE_P2P_NAN_SYNC_TLV:
			return "NAN Sync";
		case APPLE_P2P_ELECTION_PARAMETERS_V2_TLV:
			return "Election Parameters v2";
		default:
			return "Unknown";
	}
}
