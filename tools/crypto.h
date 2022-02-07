#ifndef __CRYPTO_TOOL_H__
#define __CRYPTO_TOOL_H__

#include <stdint.h>
#include <stddef.h>

#define get_bit(a, offset) (1 & ((a)>>(offset)))
#define set_bit(a, offset) ((a) |= (1<<(offset)))
#define clr_bit(a, offset) ((a) &= ~(1<<(offset)))

extern uint16_t crypto_crc_ccitt(uint16_t crc, char *buffer, int len);
extern uint16_t crypto_crc_modbus(uint16_t crc, char *buffer, int len);
extern int crypto_base64_encode(uint8_t *data,int data_len,char *output,int output_len);
extern int crypto_base64_decode(uint8_t *data,int data_len,uint8_t *output,int *output_len);
extern void crypto_sha1_sum(char *data,int data_len,uint32_t *digest);
extern void crypto_sha256_sum(char *data,size_t data_len,uint32_t *digest);

#endif	/* __CRYPTO_TOOL_H__ */