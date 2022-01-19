#include <stdio.h>
#include <string.h>
#include <netinet/in.h>

#include "crypto.h"

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