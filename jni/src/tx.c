#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <radiotap.h>

#include "tx.h"
#include "sync.h"
#include "ieee80211.h"

#include "crc32.h"
#include "response.h"
#include "log.h"
#define APPLE_P2P_DNS_SHORT_LOCAL  0xc00c  /* .local */

#define APPLE_P2P_DATA_HEAD        0x0403
#define APPLE_P2P_DATA_PAD         0x0000

#define APPLE_P2P_SOCIAL_CHANNEL_6_BIT   0x0001
#define APPLE_P2P_SOCIAL_CHANNEL_44_BIT  0x0002
#define APPLE_P2P_SOCIAL_CHANNEL_149_BIT 0x0004

int apple_p2p_init_data(uint8_t *buf, struct apple_p2p_state *state)
{
    struct apple_p2p_data *header = (struct apple_p2p_data *) buf;

    header->head = htole16(APPLE_P2P_DATA_HEAD);
    header->seq = htole16(apple_p2p_state_next_sequence_number(state));
    header->pad = htole16(APPLE_P2P_DATA_PAD);
    /* TODO we should generally to able to send any kind of data */
    header->ethertype = htobe16(ETH_P_IPV6);

    return sizeof(struct apple_p2p_data);
}

int apple_p2p_init_action(uint8_t *buf, enum apple_p2p_action_type type)
{
    struct apple_p2p_action *af = (struct apple_p2p_action *) buf;

    uint32_t steady_time = (uint32_t) clock_time_us(); /* TODO apple_p2p_init_* methods should not call clock_time_us() themselves but rather be given as parameter to avoid inconsistent calculation of stuff, e.g., fields in sync parameters should be calculated based on the timestamp indicated in af->target_tx */
    af->category = IEEE80211_VENDOR_SPECIFIC; /* vendor specific */
    af->oui = APPLE_P2P_OUI;
    af->type = APPLE_P2P_TYPE;
    af->version = APPLE_P2P_VERSION_COMPAT;
    af->subtype = type;
    af->reserved = 0;
    af->phy_tx = htole32(steady_time); /* TODO arbitrary offset */
    af->target_tx = htole32(steady_time);

    return sizeof(struct apple_p2p_action);
}

int apple_p2p_chan_encoding_length(enum apple_p2p_chan_encoding enc)
{
    switch(enc)
    {
        case APPLE_P2P_CHAN_ENC_SIMPLE:
            return 1;
        case APPLE_P2P_CHAN_ENC_LEGACY:
        case APPLE_P2P_CHAN_ENC_OPCLASS:
            return 2;
        default:
            return 0; /* unknown encoding */
    }
}

int apple_p2p_init_chanseq(uint8_t *buf, const struct apple_p2p_state *state)
{
    struct apple_p2p_chanseq *chanseq = (struct apple_p2p_chanseq *) buf;
    int enc_len = apple_p2p_chan_encoding_length(state->channel.enc);
    int offset = sizeof(struct apple_p2p_chanseq);

    chanseq->count = APPLE_P2P_CHANSEQ_LENGTH - 1; /* +1 */
    chanseq->encoding = state->channel.enc;
    chanseq->duplicate_count = 0;
    chanseq->step_count = 3;
    chanseq->fill_channel = htole16(0xffff); /* repeat current channel */
    for(int i = 0; i < APPLE_P2P_CHANSEQ_LENGTH; i++)
    {
        /* copy based on length */
        memcpy(buf + offset, &(state->channel.sequence[i]), enc_len);
        offset += enc_len;
    }
    return offset;
}

int apple_p2p_init_sync_params_tlv(uint8_t *buf, const struct apple_p2p_state *state)
{
    struct apple_p2p_sync_params_tlv *tlv = (struct apple_p2p_sync_params_tlv *) buf;
    int len;
    uint64_t now;

    now = clock_time_us();

    tlv->type = APPLE_P2P_SYNCHRONIZATON_PARAMETERS_TLV;

    /* static info: appears to be fixed in current APPLE_P2P implementation */
    tlv->guard_time = 0;
    tlv->aw_period = htole16(state->sync.aw_period);
    tlv->aw_ext_length = htole16(state->sync.aw_period);
    tlv->aw_com_length = htole16(state->sync.aw_period);
    tlv->min_ext = state->sync.presence_mode - 1;
    tlv->max_ext_unicast = state->sync.presence_mode - 1;
    tlv->max_ext_multicast = state->sync.presence_mode - 1;
    tlv->max_ext_af = state->sync.presence_mode - 1;

    tlv->flags = htole16(0x1800); /* TODO: not sure what they do */

    tlv->reserved = 0;

    /* TODO: dynamic info needs to be adjusted during runtime */
    tlv->next_aw_channel = apple_p2p_chan_num(state->channel.current,
                           state->channel.enc); /* TODO need to calculate this from current seq */
    tlv->next_aw_seq = htole16(apple_p2p_sync_current_aw(now, &state->sync));
    tlv->ap_alignment = tlv->next_aw_seq; /* apple_p2p_fill_sync_params(..) */
    tlv->tx_down_counter = htole16(apple_p2p_sync_next_aw_tu(now, &state->sync));
    tlv->af_period = htole16(state->psf_interval);
    tlv->presence_mode = state->sync.presence_mode; /* always available, usually 4 */
    tlv->master_addr = state->election.master_addr;
    tlv->master_channel = apple_p2p_chan_num(state->channel.master, state->channel.enc);
    /* lower bound is 0 */
    if(le16toh(tlv->aw_com_length) < (le16toh(tlv->aw_period) * tlv->presence_mode - le16toh(tlv->tx_down_counter)))
    { tlv->remaining_aw_length = htole16(0); }
    else
        tlv->remaining_aw_length = htole16(le16toh(tlv->aw_com_length) -
                                           (le16toh(tlv->aw_period) * tlv->presence_mode - le16toh(tlv->tx_down_counter)));

    len = sizeof(struct apple_p2p_sync_params_tlv);
    len += apple_p2p_init_chanseq(buf + sizeof(struct apple_p2p_sync_params_tlv), state);
    /* padding */
    buf[len++] = 0;
    buf[len++] = 0;

    tlv->length = htole16(len - sizeof(struct tl));

    return len;
}

int apple_p2p_init_chanseq_tlv(uint8_t *buf, const struct apple_p2p_state *state)
{
    struct apple_p2p_chanseq_tlv *tlv = (struct apple_p2p_chanseq_tlv *) buf;
    int len;

    tlv->type = APPLE_P2P_CHAN_SEQ_TLV;

    len = sizeof(struct apple_p2p_chanseq_tlv);
    len += apple_p2p_init_chanseq(buf + sizeof(struct apple_p2p_chanseq_tlv), state);
    /* padding */
    buf[len++] = 0;
    buf[len++] = 0;
    buf[len++] = 0;
    tlv->length = htole16(len - sizeof(struct tl));

    return len;
}

int apple_p2p_init_election_params_tlv(uint8_t *buf, const struct apple_p2p_state *state)
{
    struct apple_p2p_election_params_tlv *tlv = (struct apple_p2p_election_params_tlv *) buf;

    tlv->type = APPLE_P2P_ELECTION_PARAMETERS_TLV;
    tlv->length = htole16(sizeof(struct apple_p2p_election_params_tlv) - sizeof(struct tl));

    tlv->distancetop = state->election.height;
    tlv->top_master_addr = state->election.master_addr;
    tlv->top_master_metric = htole32(state->election.master_metric);
    tlv->self_metric = htole32(state->election.self_metric);

    /* TODO unsure what flags do */
    tlv->flags = 0;
    tlv->id = htole16(0);

    tlv->unknown = 0;
    tlv->pad[0] = 0;
    tlv->pad[1] = 0;

    return sizeof(struct apple_p2p_election_params_tlv);
}

int apple_p2p_init_election_params_v2_tlv(uint8_t *buf, const struct apple_p2p_state *state)
{
    struct apple_p2p_election_params_v2_tlv *tlv = (struct apple_p2p_election_params_v2_tlv *) buf;

    tlv->type = APPLE_P2P_ELECTION_PARAMETERS_V2_TLV;
    tlv->length = htole16(sizeof(struct apple_p2p_election_params_v2_tlv) - sizeof(struct tl));

    tlv->master_addr = state->election.master_addr;
    tlv->sync_addr = state->election.sync_addr;
    tlv->distance_to_master = htole32(state->election.height);
    tlv->master_metric = htole32(state->election.master_metric);
    tlv->self_metric = htole32(state->election.self_metric);
    tlv->unknown = htole32(0);
    tlv->reserved = htole32(0);

    tlv->master_counter = htole32(state->election.master_counter);
    tlv->self_counter = htole32(state->election.self_counter);

    return sizeof(struct apple_p2p_election_params_v2_tlv);
}

int apple_p2p_init_service_params_tlv(uint8_t *buf, const struct apple_p2p_state *state __attribute__((unused)))
{
    struct apple_p2p_service_params_tlv *tlv = (struct apple_p2p_service_params_tlv *) buf;

    tlv->type = APPLE_P2P_SERVICE_PARAMETERS_TLV;
    tlv->length = htole16(sizeof(struct apple_p2p_service_params_tlv) - sizeof(struct tl));

    /* changing the SUI causes receiving nodes to flush their mDNS cache,
     * other than that, this TLV seems to be deprecated in APPLE_P2P v2+ */

    tlv->unknown[0] = 0;
    tlv->unknown[1] = 0;
    tlv->unknown[2] = 0;
    tlv->sui = htole16(0);
    tlv->bitmask = htole32(0);

    return sizeof(struct apple_p2p_service_params_tlv);
}

int apple_p2p_init_ht_capabilities_tlv(uint8_t *buf, const struct apple_p2p_state *state __attribute__((unused)))
{
    struct apple_p2p_ht_capabilities_tlv *tlv = (struct apple_p2p_ht_capabilities_tlv *) buf;

    tlv->type = APPLE_P2P_ENHANCED_DATA_RATE_CAPABILITIES_TLV;
    tlv->length = htole16(sizeof(struct apple_p2p_ht_capabilities_tlv) - sizeof(struct tl));

    /* TODO extract capabilities from nl80211 (nl80211_band_attr) */
    tlv->ht_capabilities = htole16(0x01ee);
    tlv->ampdu_params = 0x19;
    tlv->rx_mcs = 0x00; /* one stream, all MCS */

    tlv->unknown = htole16(0);
    tlv->unknown2 = htole16(0);

    return sizeof(struct apple_p2p_ht_capabilities_tlv);
}

int apple_p2p_init_data_path_state_tlv(uint8_t *buf, const struct apple_p2p_state *state)
{
    struct apple_p2p_data_path_state_tlv *tlv = (struct apple_p2p_data_path_state_tlv *) buf;

    tlv->type = APPLE_P2P_DATA_PATH_STATE_TLV;
    tlv->length = htole16(sizeof(struct apple_p2p_data_path_state_tlv) - sizeof(struct tl));

    /* TODO this is very ugly currently */
    tlv->flags = htole16(0x8f64);

    tlv->apple_p2p_addr = state->self_address;

    tlv->country_code[0] = 'C';
    tlv->country_code[1] = 'N';
    tlv->country_code[2] = 0;

    if(apple_p2p_chan_num(state->channel.master, APPLE_P2P_CHAN_ENC_OPCLASS) == 6)
    { tlv->social_channels = htole16(APPLE_P2P_SOCIAL_CHANNEL_6_BIT); }
    else if(apple_p2p_chan_num(state->channel.master, APPLE_P2P_CHAN_ENC_OPCLASS) == 44)
    {
        tlv->social_channels = htole16(APPLE_P2P_SOCIAL_CHANNEL_44_BIT);
    }
    else     // 149
    {
        tlv->social_channels = htole16(APPLE_P2P_SOCIAL_CHANNEL_149_BIT);
    }

    tlv->ext_flags = htole16(0x0000);

    return sizeof(struct apple_p2p_data_path_state_tlv);
}

int apple_p2p_init_arpa_tlv(uint8_t *buf, const struct apple_p2p_state *state)
{
    struct apple_p2p_arpa_tlv *tlv = (struct apple_p2p_arpa_tlv *) buf;
    size_t len = strlen(state->name);

    tlv->type = APPLE_P2P_ARPA_TLV;
    tlv->length = htole16((uint16_t)(sizeof(struct apple_p2p_arpa_tlv) - sizeof(struct tl) + len + 1));

    tlv->flags = 3;
    tlv->name_length = (uint8_t) len;
    memcpy(tlv->name, state->name, len);
    *(uint16_t *)(tlv->name + len) = htobe16(APPLE_P2P_DNS_SHORT_LOCAL);

    return sizeof(struct tl) + le16toh(tlv->length);
}

int apple_p2p_init_service_response_tlv(uint8_t *buf, uint8_t *response, unsigned int response_length)
{
    struct tl *tl = (struct tl *)buf;

    tl->type = APPLE_P2P_SERVICE_RESPONSE_TLV;
    tl->length = response_length;
    memcpy(buf + sizeof(struct tl), response, response_length);

    return sizeof(struct tl) + le16toh(tl->length);
}

int apple_p2p_init_version_tlv(uint8_t *buf, const struct apple_p2p_state *state)
{
    struct apple_p2p_version_tlv *tlv = (struct apple_p2p_version_tlv *) buf;

    tlv->type = APPLE_P2P_VERSION_TLV;
    tlv->length = htole16(sizeof(struct apple_p2p_version_tlv) - sizeof(struct tl));
    tlv->version = state->version;
    tlv->devclass = state->dev_class;

    return sizeof(struct apple_p2p_version_tlv);
}

int ieee80211_init_radiotap_header(uint8_t *buf, const struct apple_p2p_state *state)
{
    /*
     * TX radiotap headers and mac80211
     * https://www.kernel.org/doc/Documentation/networking/mac80211-injection.txt
     */
    struct ieee80211_radiotap_header *hdr = (struct ieee80211_radiotap_header *) buf;
    uint8_t *ptr = buf + sizeof(struct ieee80211_radiotap_header);
    uint32_t present = 0;

    hdr->it_version = 0;
    hdr->it_pad = 0;

    /* TODO Adjust PHY parameters based on receiver capabilities */

    present |= ieee80211_radiotap_type_to_mask(IEEE80211_RADIOTAP_FLAGS);
    *ptr = 0;
    ptr += sizeof(uint8_t);

    present |= ieee80211_radiotap_type_to_mask(IEEE80211_RADIOTAP_RATE);
    *ptr = ieee80211_radiotap_rate_to_val(12);
    ptr += sizeof(uint8_t);

    present |= ieee80211_radiotap_type_to_mask(IEEE80211_RADIOTAP_CHANNEL);
    *(uint16_t *)ptr = htole16(ieee80211_channel_to_frequency(apple_p2p_chan_num(state->channel.master, state->channel.enc)));
    ptr += sizeof(uint16_t);
    *(uint16_t *)ptr = htole16(IEEE80211_CHAN_OFDM | IEEE80211_CHAN_5GHZ);
    ptr += sizeof(uint16_t);

    present |= ieee80211_radiotap_type_to_mask(IEEE80211_RADIOTAP_DBM_ANTSIGNAL);
    *ptr = (int8_t) -30;
    ptr += sizeof(uint8_t);

    present |= ieee80211_radiotap_type_to_mask(IEEE80211_RADIOTAP_ANTENNA);
    *ptr = 0;
    ptr += sizeof(uint8_t);

    present |= ieee80211_radiotap_type_to_mask(IEEE80211_RADIOTAP_RX_FLAGS);
    *(uint16_t *)ptr = 0;
    ptr += sizeof(uint16_t);

    hdr->it_len = htole16((uint16_t)(ptr - buf));
    hdr->it_present = htole32(present);

    return ptr - buf;
}

int ieee80211_init_apple_p2p_hdr(uint8_t *buf, const struct ether_addr *src, const struct ether_addr *dst,
                                 struct ieee80211_state *state, uint16_t type)
{
    struct ieee80211_hdr *hdr = (struct ieee80211_hdr *) buf;

    hdr->frame_control = htole16(type);
    hdr->duration_id = htole16(0);
    hdr->addr1 = *dst;
    hdr->addr2 = *src;
    hdr->addr3 = APPLE_P2P_BSSID;
    hdr->seq_ctrl = htole16(ieee80211_state_next_sequence_number(state) << 4);

    return sizeof(struct ieee80211_hdr);
}

int ieee80211_init_apple_p2p_action_hdr(uint8_t *buf, const struct ether_addr *src, const struct ether_addr *dst,
                                        struct ieee80211_state *state)
{
    return ieee80211_init_apple_p2p_hdr(buf, src, dst, state, IEEE80211_FTYPE_MGMT | IEEE80211_STYPE_ACTION);
}

int ieee80211_init_apple_p2p_data_hdr(uint8_t *buf, const struct ether_addr *src, const struct ether_addr *dst,
                                      struct ieee80211_state *state)
{
    return ieee80211_init_apple_p2p_hdr(buf, src, dst, state, IEEE80211_FTYPE_DATA | IEEE80211_STYPE_DATA);
}

int llc_init_apple_p2p_hdr(uint8_t *buf)
{
    struct llc_hdr *hdr = (struct llc_hdr *) buf;

    hdr->dsap = 0xaa; /* 0xaa - SNAP Extension Used */
    hdr->ssap = 0xaa; /* 0xaa - SNAP Extension Used */
    hdr->control = 0x03;
    hdr->oui = APPLE_P2P_OUI;
    hdr->pid = htobe16(APPLE_P2P_LLC_PROTOCOL_ID);

    return sizeof(struct llc_hdr);
}

int ieee80211_add_fcs(const uint8_t *start, uint8_t *end)
{
    uint32_t crc = crc32(start, end - start);
    *(uint32_t *) end = htole32(crc);
    return sizeof(uint32_t);
}

int apple_p2p_init_full_action_frame(uint8_t *buf, struct apple_p2p_state *state, struct ieee80211_state *ieee80211_state,
                                     enum apple_p2p_action_type type)
{
    uint8_t *ptr = buf;
    uint8_t airplay_ptr_data[1024] = { 0 };
    uint8_t raop_ptr_data[1024] = { 0 };
    uint8_t airplay_srv_data[1024] = { 0 };
    uint8_t raop_srv_data[1024] = { 0 };
    uint8_t airplay_txt_data[1024] = { 0 };
    uint8_t raop_txt_data[1024] = { 0 };
    char *name = state->name;
    uint8_t *mac = (unsigned char *) & (state->self_address.ether_addr_octet);
    char mac_str[16] = "";
    sprintf(mac_str, "%02x%02x%02x%02x%02x%02x", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
//    10:2c:6b:fd:59:96
//    78:8a:86:44:20:a5
//    78 8a864420a5
//    log_error("mac_str name %s",mac_str);
//    log_error("device name %s",name);
    uint32_t airplay_ptr_data_len = getPtrData(airplay_ptr_data, name, APPLE_P2P_DNS_SHORT_AIRPLAY_TCP, NULL);
    uint32_t raop_ptr_data_len = getPtrData(raop_ptr_data, name, APPLE_P2P_DNS_SHORT_RAOP_TCP, mac_str);
    uint32_t airplay_srv_data_len = getSrvData(airplay_srv_data, name, APPLE_P2P_DNS_SHORT_AIRPLAY_TCP, NULL, 7000);
    uint32_t raop_srv_data_len = getSrvData(raop_srv_data, name, APPLE_P2P_DNS_SHORT_RAOP_TCP, mac_str, 5000);
    uint32_t airplay_txt_data_len = getTxtData(airplay_txt_data, name, APPLE_P2P_DNS_SHORT_AIRPLAY_TCP, NULL, airplay_txt, airplay_txt_count);
    uint32_t raop_txt_data_len = getTxtData(raop_txt_data, name, APPLE_P2P_DNS_SHORT_RAOP_TCP, mac_str, raop_txt, raop_txt_count);

    ptr += ieee80211_init_radiotap_header(ptr, state);
    ptr += ieee80211_init_apple_p2p_action_hdr(ptr, &state->self_address, &state->dst, ieee80211_state);
    ptr += apple_p2p_init_action(ptr, type);
    ptr += apple_p2p_init_sync_params_tlv(ptr, state);
    ptr += apple_p2p_init_election_params_tlv(ptr, state);
    ptr += apple_p2p_init_chanseq_tlv(ptr, state);
    ptr += apple_p2p_init_election_params_v2_tlv(ptr, state);
    ptr += apple_p2p_init_service_params_tlv(ptr, state);
    if(type == APPLE_P2P_ACTION_MIF)
    {ptr += apple_p2p_init_ht_capabilities_tlv(ptr, state); }
    if(type == APPLE_P2P_ACTION_MIF)
    { ptr += apple_p2p_init_arpa_tlv(ptr, state); }
    ptr += apple_p2p_init_data_path_state_tlv(ptr, state);
    ptr += apple_p2p_init_version_tlv(ptr, state);
    #if 1
    if(type == APPLE_P2P_ACTION_MIF)
    {
        ptr += apple_p2p_init_service_response_tlv(ptr, airplay_ptr_data, airplay_ptr_data_len);
        ptr += apple_p2p_init_service_response_tlv(ptr, raop_ptr_data, raop_ptr_data_len);
        ptr += apple_p2p_init_service_response_tlv(ptr, airplay_srv_data, airplay_srv_data_len);
        ptr += apple_p2p_init_service_response_tlv(ptr, raop_srv_data, raop_srv_data_len);
        ptr += apple_p2p_init_service_response_tlv(ptr, airplay_txt_data, airplay_txt_data_len);
        ptr += apple_p2p_init_service_response_tlv(ptr, raop_txt_data, raop_txt_data_len);
    }
    #endif
    if(ieee80211_state->fcs)
    { ptr += ieee80211_add_fcs(buf, ptr); }

    return ptr - buf;
}

int apple_p2p_init_full_data_frame(uint8_t *buf, const struct ether_addr *src, const struct ether_addr *dst,
                                   const uint8_t *payload, unsigned int plen,
                                   struct apple_p2p_state *state, struct ieee80211_state *ieee80211_state)
{
    uint8_t *ptr = buf;

    ptr += ieee80211_init_radiotap_header(ptr, state);
    ptr += ieee80211_init_apple_p2p_data_hdr(ptr, src, dst, ieee80211_state);
    ptr += llc_init_apple_p2p_hdr(ptr);
    ptr += apple_p2p_init_data(ptr, state);

    memcpy(ptr, payload, plen);
    ptr += plen;
    if(ieee80211_state->fcs)
    { ptr += ieee80211_add_fcs(buf, ptr); }

    return ptr - buf;
}
