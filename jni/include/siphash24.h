#ifndef __SIPHASH24_H__
#define __SIPHASH24_H__

#define siphash24_BYTES 8U
#define siphash24_KEYBYTES 16U

int siphash24(unsigned char *out, const unsigned char *in,
              unsigned long long inlen, const unsigned char *k);

#endif /* __SIPHASH24_H__ */
