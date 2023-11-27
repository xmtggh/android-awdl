#pragma once
#include <stdint.h>
#pragma pack(1)

typedef struct service_response_s
{
    uint16_t hlen;
    uint8_t type;
    uint32_t payload_len;
    uint16_t priority;
    uint16_t weight;
    uint16_t Port;
} service_response_t;

#define SERVICE_RESPONSE_TYPE_PTR 12
#define SERVICE_RESPONSE_TYPE_SRV 33
#define SERVICE_RESPONSE_TYPE_TXT 16

#define APPLE_P2P_DNS_SHORT_NULL         0xc000  /* NULL */
#define APPLE_P2P_DNS_SHORT_AIRPLAY_TCP  0xc001  /* _airplay._tcp.local */
#define APPLE_P2P_DNS_SHORT_AIRPLAY_UDP  0xc002  /* _airplay._udp.local */
#define APPLE_P2P_DNS_SHORT_AIRPLAY      0xc003  /* _airplay */
#define APPLE_P2P_DNS_SHORT_RAOP_TCP     0xc004  /* _raop._tcp.local */
#define APPLE_P2P_DNS_SHORT_RAOP_UDP     0xc005  /* _raop._udp.local */
#define APPLE_P2P_DNS_SHORT_RAOP         0xc006  /* _raop */
#define APPLE_P2P_DNS_SHORT_AIRDROP_TCP  0xc007  /* _airdrop._tcp.local */
#define APPLE_P2P_DNS_SHORT_AIRDROP_UDP  0xc008  /* _airdrop._udp.local */
#define APPLE_P2P_DNS_SHORT_AIRDROP      0xc009  /* _airdrop */
#define APPLE_P2P_DNS_SHORT_TCP_LOCAL    0xc00a  /* _tcp.local */
#define APPLE_P2P_DNS_SHORT_UDP_LOCAL    0xc00b  /* _udp.local */
#define APPLE_P2P_DNS_SHORT_LOCAL        0xc00c  /* local */
#define APPLE_P2P_DNS_SHORT_IPV6         0xc00d  /* ip6.arpa */
#define APPLE_P2P_DNS_SHORT_IPV4         0xc00e  /* ip4.arpa */

extern const char *airplay_txt[];
extern unsigned int airplay_txt_count;
extern const char *raop_txt[];
extern unsigned int raop_txt_count;

extern uint32_t getPtrData(uint8_t *out, const char *name, uint16_t short_name, const char *mac_str);
extern uint32_t getSrvData(uint8_t *out, const char *name, uint16_t short_name, const char *mac_str, uint16_t port);
extern uint32_t getTxtData(uint8_t *out, const char *name, uint16_t short_name, const char *mac_str, const char **txt, unsigned int txt_count);
