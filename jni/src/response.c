#include "response.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>

#define p2pcast 1
#if(p2pcast == 0)
//hostname = [TinaLinux.local]
//address = [192.168.100.133]
//port = [7000]
//txt = ["wechat_flag=1" "vv=2" "pi=b08f5a79-db29-4384-b456-a4784d9e6055" "pk=0271f30cfac8cbe20b8be17e7396bf7e2bdd994f3cec916536bcc0815336f553" "flags=0x4" "model=AppleTV3,2" "features=0x5a7feff7" "deviceid=02:2e:2d:35:b0:61" "srcvers=220.68"]
const char *airplay_txt[] =
{
    "wechat_flag=1",
    "vv=2",
    "pi=b08f5a79-db29-4384-b456-a4784d9e6055",
    "pk=2c96727cd27c81789e009ae79a0edb52cf92cab68eb486792384b54c30ffdc92",
    "flags=0x4",
    "model=AppleTV3,2",
    "features=0x5A7FEFF7",
    "deviceid=02:2e:2d:35:b0:61",
    "srcvers=220.68"
};
unsigned int airplay_txt_count = 9;
#else
const char *airplay_txt[] =
{
    "srcvers=220.68",
    "deviceid=94:a4:08:b5:31:56",
    "features=0x1E,0x5A7FEFE4",
    "model=AppleTV3,2",
    "flags=0x4",
    "pk=3b6a27bcceb6a42d62a3a8d02a6f0d73653215771de243a63ac048a18b59da29",
    "pi=b08f5a79-db29-4384-b456-022e2db43a09",
    "vv=2"
};
unsigned int airplay_txt_count = 8;
#endif
#if(p2pcast == 0)
//hostname = [TinaLinux.local]
//address = [192.168.100.133]
//port = [5000]
//txt = ["pk=0271f30cfac8cbe20b8be17e7396bf7e2bdd994f3cec916536bcc0815336f553" "am=Shairport,1" "sf=0x4" "ek=1" "sm=false" "vs=220.68" "md=0,1,2" "tp=UDP" "vn=3" "pw=false" "ss=16" "sr=44100" "da=true" "sv=false" "et=0,3,5" "cn=0,1,3" "ch=2" "txtvers=1"]
const char *raop_txt[] =
{
    "pk=2c96727cd27c81789e009ae79a0edb52cf92cab68eb486792384b54c30ffdc92",
    "am=Shairport,1",
    "sf=0x4",
    "ek=1",
    "sm=false",
    "vs=220.68",
    "md=0,1,2",
    "tp=UDP",
    "vn=3",
    "pw=false",
    "ss=16",
    "sr=44100",
    "da=true",
    "sv=false",
    "et=0,3,5",
    "cn=0,1,3",
    "ch=2",
    "txtvers=1"
};
#else
const char *raop_txt[] =
{
    "txtvers=1",
    "ch=2",
    "cn=0,1,3",
    "et=0,3,5",
    "sv=false",
    "da=true",
    "sr=44100",
    "ss=16",
    "pw=true",
    "vn=3",
    "tp=UDP",
    "md=0,1,2",
    "vs=220.68",
    "sm=false",
    "ek=1",
    "sf=0x4",
    "am=Shairport,1",
    "pk=3b6a27bcceb6a42d62a3a8d02a6f0d73653215771de243a63ac048a18b59da29"
};
#endif
unsigned int raop_txt_count = 18;

#if TEST
char *bin2hex(const unsigned char *buf, int len)
{
    int i;
    char *hex = (char *)malloc(len * 2 + 1);
    for(i = 0; i < len * 2; i++)
    {
        int val = (i % 2) ? buf[i / 2] & 0x0f : (buf[i / 2] & 0xf0) >> 4;
        hex[i] = (val < 10) ? '0' + val : 'a' + (val - 10);
    }
    hex[len * 2] = 0;
    return hex;
}
#endif

uint32_t append_data(uint8_t *buf, uint32_t offset, uint8_t *data, uint32_t len)
{
    memcpy(&buf[offset], data, len);
    return len;
}

uint32_t append_8bit(uint8_t *buf, uint32_t offset, uint8_t value)
{
    buf[offset] = value;
    return 1;
}

uint32_t append_16bit(uint8_t *buf, uint32_t offset, uint16_t value)
{
    uint16_t *p = (uint16_t *)(&buf[offset]);
    *p = value;
    return 2;
}

uint32_t append_32bit(uint8_t *buf, uint32_t offset, uint32_t value)
{
    uint32_t *p = (uint32_t *)(&buf[offset]);
    *p = value;
    return 4;
}

uint32_t getPtrData(uint8_t *out, const char *name, uint16_t short_name, const char *mac_str)
{
    service_response_t rsp = { 0 };
    uint32_t outLen = 0;
    char domain[64];
    if(mac_str)
    {
        sprintf(domain, "%s@%s", mac_str, name);
    }
    else
    {
        strcpy(domain, name);
    }

    rsp.type = SERVICE_RESPONSE_TYPE_PTR;
    rsp.hlen = sizeof(short_name) + sizeof(rsp.type);

    rsp.payload_len = strlen(domain) + 3;

    outLen += append_16bit(out, outLen, rsp.hlen);
    outLen += append_16bit(out, outLen, htons(short_name));
    outLen += append_8bit(out, outLen, rsp.type);

    outLen += append_32bit(out, outLen, rsp.payload_len);
    outLen += append_8bit(out, outLen, (uint8_t)strlen(domain));
    outLen += append_data(out, outLen, (uint8_t *)domain, strlen(domain));
    outLen += append_16bit(out, outLen, htons(APPLE_P2P_DNS_SHORT_NULL));

    #if TEST
    printf("%s\n", bin2hex(out, outLen));
    #endif

    return outLen;
}

uint32_t getSrvData(uint8_t *out, const char *name, uint16_t short_name, const char *mac_str, uint16_t port)
{
    service_response_t rsp = { 0 };
    uint32_t outLen = 0;
    char domain[64];
    if(mac_str)
    {
        sprintf(domain, "%s@%s", mac_str, name);
    }
    else
    {
        strcpy(domain, name);
    }

    rsp.type = SERVICE_RESPONSE_TYPE_SRV;
    rsp.priority = 0;
    rsp.weight = 0;
    rsp.Port = htons(port);
    rsp.hlen = strlen(domain) + 3 + sizeof(rsp.type);
    rsp.payload_len = 6 + strlen(name) + 3;

    outLen += append_16bit(out, outLen, rsp.hlen);
    outLen += append_8bit(out, outLen, (uint8_t)strlen(domain));
    outLen += append_data(out, outLen, (uint8_t *)domain, strlen(domain));
    outLen += append_16bit(out, outLen, htons(short_name));
    outLen += append_8bit(out, outLen, rsp.type);

    outLen += append_32bit(out, outLen, rsp.payload_len);
    outLen += append_16bit(out, outLen, rsp.priority);
    outLen += append_16bit(out, outLen, rsp.weight);
    outLen += append_16bit(out, outLen, rsp.Port);
    outLen += append_8bit(out, outLen, (uint8_t)strlen(name));
    outLen += append_data(out, outLen, (uint8_t *)name, strlen(name));
    outLen += append_16bit(out, outLen, htons(APPLE_P2P_DNS_SHORT_LOCAL));

    #if TEST
    printf("%s\n", bin2hex(out, outLen));
    #endif

    return outLen;
}

uint32_t getTxtData(uint8_t *out, const char *name, uint16_t short_name, const char *mac_str, const char **txt, unsigned int txt_count)
{
    unsigned int i;
    service_response_t rsp = { 0 };
    uint32_t outLen = 0;
    char domain[64];
    if(mac_str)
    {
        sprintf(domain, "%s@%s", mac_str, name);
    }
    else
    {
        strcpy(domain, name);
    }

    rsp.type = SERVICE_RESPONSE_TYPE_TXT;
    rsp.hlen = strlen(domain) + 3 + sizeof(rsp.type);
    for(i = 0; i < txt_count; i++)
    {
        rsp.payload_len += strlen(txt[i]) + 1;
    }

    outLen += append_16bit(out, outLen, rsp.hlen);
    outLen += append_8bit(out, outLen, (uint8_t)strlen(domain));
    outLen += append_data(out, outLen, (uint8_t *)domain, strlen(domain));
    outLen += append_16bit(out, outLen, htons(short_name));
    outLen += append_8bit(out, outLen, rsp.type);

    outLen += append_32bit(out, outLen, rsp.payload_len);
    for(i = 0; i < txt_count; i++)
    {
        outLen += append_8bit(out, outLen, (uint8_t)strlen(txt[i]));
        outLen += append_data(out, outLen, (uint8_t *)txt[i], strlen(txt[i]));
    }

    #if TEST
    printf("%s\n", bin2hex(out, outLen));
    #endif

    return outLen;
}

#if TEST
int main(int argc, char **argv)
{
    uint8_t airplay_ptr_data[1024] = { 0 };
    uint8_t raop_ptr_data[1024] = { 0 };
    uint8_t airplay_srv_data[1024] = { 0 };
    uint8_t raop_srv_data[1024] = { 0 };
    uint8_t airplay_txt_data[1024] = { 0 };
    uint8_t raop_txt_data[1024] = { 0 };
    char name[] = "PazzPort-160553-ITV";
    char mac_str[] = "022e2d35b061";
    getPtrData(airplay_ptr_data, name, APPLE_P2P_DNS_SHORT_AIRPLAY_TCP, NULL);
    getPtrData(raop_ptr_data, name, APPLE_P2P_DNS_SHORT_AIRPLAY_TCP, mac_str);
    getSrvData(airplay_srv_data, name, APPLE_P2P_DNS_SHORT_AIRPLAY_TCP, NULL, 7000);
    getSrvData(raop_srv_data, name, APPLE_P2P_DNS_SHORT_RAOP_TCP, mac_str, 5000);
    getTxtData(airplay_txt_data, name, APPLE_P2P_DNS_SHORT_AIRPLAY_TCP, NULL, airplay_txt, airplay_txt_count);
    getTxtData(raop_txt_data, name, APPLE_P2P_DNS_SHORT_AIRPLAY_TCP, mac_str, raop_txt, raop_txt_count);

    return 0;
}
#endif
