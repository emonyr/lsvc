#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include "crypto.h"

/*
 * @ccitt table
 */
uint16_t const crc_ccitt_table[256] = {
	0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7, 
	0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF, 
	0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6, 
	0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE, 
	0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485, 
	0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D, 
	0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4, 
	0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC, 
	0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823, 
	0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B, 
	0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12, 
	0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A, 
	0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41, 
	0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49, 
	0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70, 
	0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78, 
	0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F, 
	0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067, 
	0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E, 
	0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256, 
	0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D, 
	0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405, 
	0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C, 
	0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634, 
	0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB, 
	0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3, 
	0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A, 
	0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92, 
	0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9, 
	0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1, 
	0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8, 
	0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};


/*
 * @modbus table
 */
uint16_t const crc_modbus_table[256] = {
	0x0000, 0xC0C1, 0xC181, 0x0140, 0xC301, 0x03C0, 0x0280, 0xC241, 
	0xC601, 0x06C0, 0x0780, 0xC741, 0x0500, 0xC5C1, 0xC481, 0x0440, 
	0xCC01, 0x0CC0, 0x0D80, 0xCD41, 0x0F00, 0xCFC1, 0xCE81, 0x0E40, 
	0x0A00, 0xCAC1, 0xCB81, 0x0B40, 0xC901, 0x09C0, 0x0880, 0xC841, 
	0xD801, 0x18C0, 0x1980, 0xD941, 0x1B00, 0xDBC1, 0xDA81, 0x1A40, 
	0x1E00, 0xDEC1, 0xDF81, 0x1F40, 0xDD01, 0x1DC0, 0x1C80, 0xDC41, 
	0x1400, 0xD4C1, 0xD581, 0x1540, 0xD701, 0x17C0, 0x1680, 0xD641, 
	0xD201, 0x12C0, 0x1380, 0xD341, 0x1100, 0xD1C1, 0xD081, 0x1040, 
	0xF001, 0x30C0, 0x3180, 0xF141, 0x3300, 0xF3C1, 0xF281, 0x3240, 
	0x3600, 0xF6C1, 0xF781, 0x3740, 0xF501, 0x35C0, 0x3480, 0xF441, 
	0x3C00, 0xFCC1, 0xFD81, 0x3D40, 0xFF01, 0x3FC0, 0x3E80, 0xFE41, 
	0xFA01, 0x3AC0, 0x3B80, 0xFB41, 0x3900, 0xF9C1, 0xF881, 0x3840, 
	0x2800, 0xE8C1, 0xE981, 0x2940, 0xEB01, 0x2BC0, 0x2A80, 0xEA41, 
	0xEE01, 0x2EC0, 0x2F80, 0xEF41, 0x2D00, 0xEDC1, 0xEC81, 0x2C40, 
	0xE401, 0x24C0, 0x2580, 0xE541, 0x2700, 0xE7C1, 0xE681, 0x2640, 
	0x2200, 0xE2C1, 0xE381, 0x2340, 0xE101, 0x21C0, 0x2080, 0xE041, 
	0xA001, 0x60C0, 0x6180, 0xA141, 0x6300, 0xA3C1, 0xA281, 0x6240, 
	0x6600, 0xA6C1, 0xA781, 0x6740, 0xA501, 0x65C0, 0x6480, 0xA441, 
	0x6C00, 0xACC1, 0xAD81, 0x6D40, 0xAF01, 0x6FC0, 0x6E80, 0xAE41, 
	0xAA01, 0x6AC0, 0x6B80, 0xAB41, 0x6900, 0xA9C1, 0xA881, 0x6840, 
	0x7800, 0xB8C1, 0xB981, 0x7940, 0xBB01, 0x7BC0, 0x7A80, 0xBA41, 
	0xBE01, 0x7EC0, 0x7F80, 0xBF41, 0x7D00, 0xBDC1, 0xBC81, 0x7C40, 
	0xB401, 0x74C0, 0x7580, 0xB541, 0x7700, 0xB7C1, 0xB681, 0x7640, 
	0x7200, 0xB2C1, 0xB381, 0x7340, 0xB101, 0x71C0, 0x7080, 0xB041, 
	0x5000, 0x90C1, 0x9181, 0x5140, 0x9301, 0x53C0, 0x5280, 0x9241, 
	0x9601, 0x56C0, 0x5780, 0x9741, 0x5500, 0x95C1, 0x9481, 0x5440, 
	0x9C01, 0x5CC0, 0x5D80, 0x9D41, 0x5F00, 0x9FC1, 0x9E81, 0x5E40, 
	0x5A00, 0x9AC1, 0x9B81, 0x5B40, 0x9901, 0x59C0, 0x5880, 0x9841, 
	0x8801, 0x48C0, 0x4980, 0x8941, 0x4B00, 0x8BC1, 0x8A81, 0x4A40, 
	0x4E00, 0x8EC1, 0x8F81, 0x4F40, 0x8D01, 0x4DC0, 0x4C80, 0x8C41, 
	0x4400, 0x84C1, 0x8581, 0x4540, 0x8701, 0x47C0, 0x4680, 0x8641, 
	0x8201, 0x42C0, 0x4380, 0x8341, 0x4100, 0x81C1, 0x8081, 0x4040
};

static uint16_t _crc_ccitt_byte(uint16_t crc, char c)
{
	return (crc << 8) ^ crc_ccitt_table[(crc >> 8 ^ c) & 0xff];
}

/*crypto_crc_ccitt()
 *@brief	recompute the CCITT CRC for the data buffer
 *@param	crc: previous CRC value
 *@param	buffer: data pointer
 *@param	len: number of bytes in the buffer
 *@return	crc: calculation result
 */
uint16_t crypto_crc_ccitt(uint16_t crc, char *buffer, int len)
{
	while (len--)
		crc = _crc_ccitt_byte(crc, *buffer++);
	return crc;
}


static uint16_t _crc_modbus_byte(uint16_t crc, char c)
{
	return (crc >> 8) ^ crc_modbus_table[((crc & 0xff) ^ c) & 0xff];
}

/*crypto_crc_modbus()
 *@brief	recompute the Modbus CRC for the data buffer
 *@param	crc: previous CRC value
 *@param	buffer: data pointer
 *@param	len: number of bytes in the buffer
 *@return	crc: calculation result
 */
uint16_t crypto_crc_modbus(uint16_t crc, char *buffer, int len)
{
	while (len--)
		crc = _crc_modbus_byte(crc, *buffer++);
	return crc;
}

/*
 * base64 character table
 *
 */

char base64_encode_table[] = {
	'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 
	'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 
	'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 
	'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 
	'w', 'x', 'y', 'z', '0', '1', '2', '3', '4', '5', '6', '7', 
	'8', '9', '+', '/',
};

char base64_decode_table[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 62, -1, -1, -1, 63, 
	52, 53, 54, 55, 56, 57, 58, 59, 60, 61, -1, -1, -1, -1, -1, -1, 
	-1,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14, 
	15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1, -1, -1, -1, 
	-1, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 
	41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
};


/*crypto_base64_encode()
 *@brief	encode data with base64 table
 *@param	data: data pointer
 *@param	data_len: data length
 *@param	output: output data pointer
 *@param	output_len: output data length
 *@return	0: on success
			-1: on failure
 */
int crypto_base64_encode(uint8_t *data,int data_len,char *output,int output_len)
{
	if(data == NULL || output == NULL || output_len < (data_len*(4/3)))
		return -1;
	
	int i,j,shift,offset;
	uint8_t cut=0;
	
	memset(output,0,output_len);
	for(i=0,j=0;i<data_len;j++){
		shift = ((j+1)%4)*2;
		if(shift != 0){
			offset = (data[i] >> shift) | (cut << (8-shift));
			output[j] = base64_encode_table[offset];
			cut = data[i] & ~(~0 << shift);
			i++;
		}else{
			output[j] = base64_encode_table[cut];
			cut = 0;
		}
	}
	
	//pading '='
	switch(shift){
		case 2:
			output[j++] = base64_encode_table[cut << 4];
			output[j++] = '=';
			output[j++] = '=';
			break;
		
		case 4:
			output[j++] = base64_encode_table[cut << 2];
			output[j++] = '=';
			break;
			
		case 6:
			output[j++] = base64_encode_table[cut << 0];
			break;
	}
	output[j] = '\0';
	
	//print_log("encode: %s\n",output);
	
	return 0;
}


/*crypto_base64_decode()
 *@brief	decode data with base64 table
 *@param	data: data pointer
 *@param	data_len: data length
 *@param	output: output data pointer
 *@param	output_len: output data length pointer(will be updated after successful decoding)
 *@return	0: on success
			-1: on failure
 */
int crypto_base64_decode(uint8_t *data,int data_len,uint8_t *output,int *output_len)
{
	if(data == NULL || output == NULL || *output_len < (data_len*(3/4)))
		return -1;
	
	int i,len,shift,decode_val;
	char cut;
	
	memset(output,0,*output_len);
	for(i=0,len=0;i<data_len;i++){
		if(data[i] == '=')
			break;
		
		decode_val = base64_decode_table[(uint8_t)data[i]];
		if(decode_val == -1)
			return -1;
		
		if(i != 0 && shift != 0)
			output[len++] = (decode_val >> (6-shift)) | cut;
		
		shift = ((i+1)%4)*2;
		cut = (decode_val & ~(~0 << (8-shift))) << shift;
	}
	*output_len = len;
	//print_log("decode: %s\n",output);	
	
	return 0;
}


/* The SHA-1 f()-functions.  */

#define SHA1_F1(x,y,z)	((x & y) ^ ((~x) & z))
#define SHA1_F2(x,y,z)   (x ^ y ^ z)			/* XOR */
#define SHA1_F3(x,y,z)	((x & y) ^ (x & z) ^ (y & z))


/* The SHA-1 Mysterious Constants */
int SHA1_K[4] = {	0x5A827999L,	/* Rounds  0-19: sqrt(2) * 2^30 */
					0x6ED9EBA1L,	/* Rounds 20-39: sqrt(3) * 2^30 */
					0x8F1BBCDCL,	/* Rounds 40-59: sqrt(5) * 2^30 */
					0xCA62C1D6L,	/* Rounds 60-79: sqrt(10) * 2^30 */};


/*
 * _rol32()
 *@brief	rotate a 32-bit value left
 *@param	word: value to rotate
 *@param	shift: bits to roll
 */
static inline uint32_t _rol32(uint32_t word, uint32_t shift)
{
	return (word << shift) | (word >> (32 - shift));
}


/*
 *_sha1_init()
 *@brief	initialize the vectors for a SHA1 digest
 *@param	buf: vector to be initialized
 */
static void _sha1_init(uint32_t *buf)
{
	buf[0] = 0x67452301;
	buf[1] = 0xefcdab89;
	buf[2] = 0x98badcfe;
	buf[3] = 0x10325476;
	buf[4] = 0xc3d2e1f0;
	
	return;
}


/*
 *_sha1_transform()
 *@brief	single block SHA1 transform
 *@param	digest: 160 bit digest to update
 *@param	in: 512 bits of data to hash
 *@param	W: 80 words of workspace
 */
static void _sha1_transform(uint32_t *digest, const char *in, uint32_t *W)
{
	uint32_t a, b, c, d, e, t, i;

	for (i = 0; i < 16; i++){
		W[i] = ntohl(((uint32_t *)in)[i]);
	}

	for (; i < 80; i++)
		W[i] = _rol32(W[i-3] ^ W[i-8] ^ W[i-14] ^ W[i-16], 1);

	a = digest[0];
	b = digest[1];
	c = digest[2];
	d = digest[3];
	e = digest[4];
	
	for (i = 0; i < 20; i++) {
		t = SHA1_F1(b, c, d) + SHA1_K[0] + _rol32(a, 5) + e + W[i];
		e = d; d = c; c = _rol32(b, 30); b = a; a = t;
	}

	for (; i < 40; i ++) {
		t = SHA1_F2(b, c, d) + SHA1_K[1] + _rol32(a, 5) + e + W[i];
		e = d; d = c; c = _rol32(b, 30); b = a; a = t;
	}

	for (; i < 60; i ++) {
		t = SHA1_F3(b, c, d) + SHA1_K[2] + _rol32(a, 5) + e + W[i];
		e = d; d = c; c = _rol32(b, 30); b = a; a = t;
	}

	for (; i < 80; i ++) {
		t = SHA1_F2(b, c, d) + SHA1_K[3] + _rol32(a, 5) + e + W[i];
		e = d; d = c; c = _rol32(b, 30); b = a; a = t;
	}
	
	digest[0] += a;
	digest[1] += b;
	digest[2] += c;
	digest[3] += d;
	digest[4] += e;
	
	return;
}


/*
 *crypto_sha1_sum()
 *@brief	calculate SHA-1 summary
 *@param	data: data to be digested
 *@param	data_len: data length
 *@param	digest: sha1 summary
 */
void crypto_sha1_sum(char *data,int data_len,uint32_t *digest)
{
	if(data == NULL)
		return;
	
	int i,buf_len=data_len+(data_len%64 >= 56 ? (128 - data_len%64) : (64 - data_len%64));
	char block[64];
	char buf[buf_len];
	uint32_t W[80];
	uint32_t tmp;
	
	//copy data into buf
	memset(buf,0,buf_len);
	memcpy(buf,data,data_len);
	//save bit count to tmp and append "1" to the end of data
	tmp = data_len*8;
	tmp = htonl(tmp);
	set_bit(buf[data_len],7);
	
	//append bit count to buf end
	*(uint32_t *)&buf[buf_len-4] = tmp;
	
	//init sha digest vector
	_sha1_init(digest);
	//devide data into blocks , 512bits(64bytes) each , transform each block
	memset(block,0,64);
	for(i=0;i<buf_len+1;i++){
		//do sha1 calculation for each block(64 bytes)
		if(i != 0 && (i%64) == 0){
			//sha transform
			_sha1_transform(digest,block,W);
			if(i == buf_len)
				break;
			//clean up for next block
			memset(block,0,64);
		}
		//copy buf into block
		block[i%64] = buf[i];
	}
	//print_log("sha-1: %x%x%x%x%x\n",digest[0],digest[1],digest[2],digest[3],digest[4]);
	
	return;
}


/*
 * _ror32()
 *@brief	rotate a 32-bit value right
 *@param	word: value to rotate
 *@param	shift: bits to roll
 */
static inline uint32_t _ror32(uint32_t word, uint32_t shift)
{
	return (word >> shift) | (word << (32 - shift));
}


/* The SHA-256 f()-functions.  */

#define SHA256_F1(x,y,z)	((x & y) ^ ((~x) & z))
#define SHA256_F2(x,y,z)	((x & y) ^ (x & z) ^ (y & z))

#define SHA256_S1(x)	(_ror32(x,2) ^ _ror32(x,13) ^ _ror32(x,22))
#define SHA256_S2(x)	(_ror32(x,6) ^ _ror32(x,11) ^ _ror32(x,25))
#define SHA256_S3(x)	(_ror32(x,7) ^ _ror32(x,18) ^ (x >> 3))
#define SHA256_S4(x)	(_ror32(x,17) ^ _ror32(x,19) ^ (x >> 10))


/* The SHA-1 Mysterious Constants */
int SHA256_K[64] = {
	0x428a2f98L, 0x71374491L, 0xb5c0fbcfL, 0xe9b5dba5L, 0x3956c25bL, 0x59f111f1L, 0x923f82a4L, 0xab1c5ed5L,
	0xd807aa98L, 0x12835b01L, 0x243185beL, 0x550c7dc3L, 0x72be5d74L, 0x80deb1feL, 0x9bdc06a7L, 0xc19bf174L,
	0xe49b69c1L, 0xefbe4786L, 0x0fc19dc6L, 0x240ca1ccL, 0x2de92c6fL, 0x4a7484aaL, 0x5cb0a9dcL, 0x76f988daL,
	0x983e5152L, 0xa831c66dL, 0xb00327c8L, 0xbf597fc7L, 0xc6e00bf3L, 0xd5a79147L, 0x06ca6351L, 0x14292967L,
	0x27b70a85L, 0x2e1b2138L, 0x4d2c6dfcL, 0x53380d13L, 0x650a7354L, 0x766a0abbL, 0x81c2c92eL, 0x92722c85L,
	0xa2bfe8a1L, 0xa81a664bL, 0xc24b8b70L, 0xc76c51a3L, 0xd192e819L, 0xd6990624L, 0xf40e3585L, 0x106aa070L,
	0x19a4c116L, 0x1e376c08L, 0x2748774cL, 0x34b0bcb5L, 0x391c0cb3L, 0x4ed8aa4aL, 0x5b9cca4fL, 0x682e6ff3L,
	0x748f82eeL, 0x78a5636fL, 0x84c87814L, 0x8cc70208L, 0x90befffaL, 0xa4506cebL, 0xbef9a3f7L, 0xc67178f2L,};


/*
 *_sha256_init()
 *@brief	initialize the vectors for a SHA1 digest
 *@param	buf: vector to initialize
 */
static void _sha256_init(uint32_t *buf)
{
	buf[0] = 0x6a09e667;
	buf[1] = 0xbb67ae85;
	buf[2] = 0x3c6ef372;
	buf[3] = 0xa54ff53a;
	buf[4] = 0x510e527f;
	buf[5] = 0x9b05688c;
	buf[6] = 0x1f83d9ab;
	buf[7] = 0x5be0cd19;
	
	return;
}


/*
 *_sha256_transform()
 *@brief	single block SHA-256 transform
 *@param	digest: 160 bit digest to update
 *@param	in: 512 bits of data to hash
 *@param	W: 80 words of workspace
 */
static void _sha256_transform(uint32_t *digest, const char *in, uint32_t *W)
{
	uint32_t a, b, c, d, e, f, g, h, t1, t2, i;

	for (i = 0; i < 16; i++)
		W[i] = ntohl(((uint32_t *)in)[i]);

	for (; i < 80; i++)
		W[i] = SHA256_S4(W[i-2]) + W[i-7] + SHA256_S3(W[i-15]) + W[i-16];
	
	a = digest[0];
	b = digest[1];
	c = digest[2];
	d = digest[3];
	e = digest[4];
	f = digest[5];
	g = digest[6];
	h = digest[7];

	for (i = 0; i < 64; i++) {
		t1 = h + SHA256_S2(e) + SHA256_F1(e,f,g) + SHA256_K[i] + W[i];
		t2 = SHA256_S1(a) + SHA256_F2(a,b,c);
		h = g; g = f; f = e; e = d + t1; d = c; c = b; b = a; a = t1 + t2;
	}
	
	digest[0] += a;
	digest[1] += b;
	digest[2] += c;
	digest[3] += d;
	digest[4] += e;
	digest[5] += f;
	digest[6] += g;
	digest[7] += h;
	
	return;
}


/*
 *crypto_sha256_sum()
 *@brief	calculate SHA-256 summary
 *@param	data: data to be digested
 *@param	data_len: data length
 *@param	digest: sha256 summary
 */
void crypto_sha256_sum(char *data,size_t data_len,uint32_t *digest)
{
	if(data == NULL)
		return;
	
	int i,buf_len=data_len+(data_len%64 >= 56 ? (128 - data_len%64) : (64 - data_len%64));
	char block[64];
	char buf[buf_len];
	uint32_t W[80];
	uint32_t tmp;
	
	//copy data into buf
	memset(buf,0,buf_len);
	memcpy(buf,data,data_len);
	//save bit count to tmp and append "1" to the end of data
	tmp = data_len*8;
	tmp = htonl(tmp);
	set_bit(buf[data_len],7);

	//append bit count to buf end
	*(uint32_t *)&buf[buf_len-4] = tmp;
	
	//init sha digest vector
	_sha256_init(digest);
	//devide data into blocks , 512bits(64bytes) each , transform each block
	memset(block,0,64);
	for(i=0;i<buf_len+1;i++){
		//do sha1 calculation for each block(64 bytes)
		if(i != 0 && (i%64) == 0){
			//sha transform
			_sha256_transform(digest,block,W);
			if(i == buf_len)
				break;
			//clean up for next block
			memset(block,0,64);
		}
		//copy buf into block
		block[i%64] = buf[i];
	}
	
	return;
}