/*
 * RFC 1321 compliant MD5 implementation
 *
 * Copyright (C) 2001-2003 Christophe Devine
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, visit the http://fsf.org website.
 */

/* rsync-3.0.6/byteorder.h */

/*
 * Simple byteorder handling.
 *
 * Copyright (C) 1992-1995 Andrew Tridgell
 * Copyright (C) 2007-2008 Wayne Davison
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, visit the http://fsf.org website.
 */


#include <inttypes.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>


#undef CAREFUL_ALIGNMENT

/* We know that the x86 can handle misalignment and has the same
 * byte order (LSB-first) as the 32-bit numbers we transmit. */

#ifdef __i386__
#define CAREFUL_ALIGNMENT 0
#endif

#ifndef CAREFUL_ALIGNMENT
#define CAREFUL_ALIGNMENT 1
#endif

#define CVAL(buf,pos) (((unsigned char *)(buf))[pos])
#define UVAL(buf,pos) ((uint32_t)CVAL(buf,pos))
#define SCVAL(buf,pos,val) (CVAL(buf,pos) = (val))

#if CAREFUL_ALIGNMENT
#define PVAL(buf,pos) (UVAL(buf,pos)|UVAL(buf,(pos)+1)<<8)
#define IVAL(buf,pos) (PVAL(buf,pos)|PVAL(buf,(pos)+2)<<16)
#define SSVALX(buf,pos,val) (CVAL(buf,pos)=(val)&0xFF,CVAL(buf,pos+1)=(val)>>8)
#define SIVALX(buf,pos,val) (SSVALX(buf,pos,val&0xFFFF),SSVALX(buf,pos+2,val>>16))
#define SIVAL(buf,pos,val) SIVALX((buf),(pos),((uint32_t)(val)))
#else

/* this handles things for architectures like the 386 that can handle
   alignment errors */

/*
   WARNING: This section is dependent on the length of int32
   being correct. set CAREFUL_ALIGNMENT if it is not.
*/

#define IVAL(buf,pos) (*(uint32_t *)((char *)(buf) + (pos)))
#define SIVAL(buf,pos,val) IVAL(buf,pos)=((uint32_t)(val))
#endif

/* The include file for both the MD4 and MD5 routines. */

#define MD5_DIGEST_LEN 16
#define MAX_DIGEST_LEN MD5_DIGEST_LEN

#define CSUM_CHUNK 64


typedef struct {
	uint32_t A, B, C, D;
	uint32_t totalN;          /* bit count, lower 32 bits */
	uint32_t totalN2;         /* bit count, upper 32 bits */
	uint8_t buffer[CSUM_CHUNK];
} md_context;

static void md5_begin(md_context *ctx);
static void md5_update(md_context *ctx, const uint8_t *input, uint32_t length);
static void md5_result(md_context *ctx, uint8_t digest[MD5_DIGEST_LEN]);


static void md5_begin(md_context *ctx)
{
	ctx->A = 0x67452301;
	ctx->B = 0xEFCDAB89;
	ctx->C = 0x98BADCFE;
	ctx->D = 0x10325476;

	ctx->totalN = ctx->totalN2 = 0;
}

static void md5_process(md_context *ctx, const uint8_t data[CSUM_CHUNK])
{
	uint32_t X[16], A, B, C, D;

	A = ctx->A;
	B = ctx->B;
	C = ctx->C;
	D = ctx->D;

	X[0] = IVAL(data, 0);
	X[1] = IVAL(data, 4);
	X[2] = IVAL(data, 8);
	X[3] = IVAL(data, 12);
	X[4] = IVAL(data, 16);
	X[5] = IVAL(data, 20);
	X[6] = IVAL(data, 24);
	X[7] = IVAL(data, 28);
	X[8] = IVAL(data, 32);
	X[9] = IVAL(data, 36);
	X[10] = IVAL(data, 40);
	X[11] = IVAL(data, 44);
	X[12] = IVAL(data, 48);
	X[13] = IVAL(data, 52);
	X[14] = IVAL(data, 56);
	X[15] = IVAL(data, 60);

#define S(x,n) ((x << n) | ((x & 0xFFFFFFFF) >> (32 - n)))

#define P(a,b,c,d,k,s,t) a += F(b,c,d) + X[k] + t, a = S(a,s) + b

#define F(x,y,z) (z ^ (x & (y ^ z)))

	P(A, B, C, D,  0,  7, 0xD76AA478);
	P(D, A, B, C,  1, 12, 0xE8C7B756);
	P(C, D, A, B,  2, 17, 0x242070DB);
	P(B, C, D, A,  3, 22, 0xC1BDCEEE);
	P(A, B, C, D,  4,  7, 0xF57C0FAF);
	P(D, A, B, C,  5, 12, 0x4787C62A);
	P(C, D, A, B,  6, 17, 0xA8304613);
	P(B, C, D, A,  7, 22, 0xFD469501);
	P(A, B, C, D,  8,  7, 0x698098D8);
	P(D, A, B, C,  9, 12, 0x8B44F7AF);
	P(C, D, A, B, 10, 17, 0xFFFF5BB1);
	P(B, C, D, A, 11, 22, 0x895CD7BE);
	P(A, B, C, D, 12,  7, 0x6B901122);
	P(D, A, B, C, 13, 12, 0xFD987193);
	P(C, D, A, B, 14, 17, 0xA679438E);
	P(B, C, D, A, 15, 22, 0x49B40821);

#undef F
#define F(x,y,z) (y ^ (z & (x ^ y)))

	P(A, B, C, D,  1,  5, 0xF61E2562);
	P(D, A, B, C,  6,  9, 0xC040B340);
	P(C, D, A, B, 11, 14, 0x265E5A51);
	P(B, C, D, A,  0, 20, 0xE9B6C7AA);
	P(A, B, C, D,  5,  5, 0xD62F105D);
	P(D, A, B, C, 10,  9, 0x02441453);
	P(C, D, A, B, 15, 14, 0xD8A1E681);
	P(B, C, D, A,  4, 20, 0xE7D3FBC8);
	P(A, B, C, D,  9,  5, 0x21E1CDE6);
	P(D, A, B, C, 14,  9, 0xC33707D6);
	P(C, D, A, B,  3, 14, 0xF4D50D87);
	P(B, C, D, A,  8, 20, 0x455A14ED);
	P(A, B, C, D, 13,  5, 0xA9E3E905);
	P(D, A, B, C,  2,  9, 0xFCEFA3F8);
	P(C, D, A, B,  7, 14, 0x676F02D9);
	P(B, C, D, A, 12, 20, 0x8D2A4C8A);

#undef F
#define F(x,y,z) (x ^ y ^ z)

	P(A, B, C, D,  5,  4, 0xFFFA3942);
	P(D, A, B, C,  8, 11, 0x8771F681);
	P(C, D, A, B, 11, 16, 0x6D9D6122);
	P(B, C, D, A, 14, 23, 0xFDE5380C);
	P(A, B, C, D,  1,  4, 0xA4BEEA44);
	P(D, A, B, C,  4, 11, 0x4BDECFA9);
	P(C, D, A, B,  7, 16, 0xF6BB4B60);
	P(B, C, D, A, 10, 23, 0xBEBFBC70);
	P(A, B, C, D, 13,  4, 0x289B7EC6);
	P(D, A, B, C,  0, 11, 0xEAA127FA);
	P(C, D, A, B,  3, 16, 0xD4EF3085);
	P(B, C, D, A,  6, 23, 0x04881D05);
	P(A, B, C, D,  9,  4, 0xD9D4D039);
	P(D, A, B, C, 12, 11, 0xE6DB99E5);
	P(C, D, A, B, 15, 16, 0x1FA27CF8);
	P(B, C, D, A,  2, 23, 0xC4AC5665);

#undef F
#define F(x,y,z) (y ^ (x | ~z))

	P(A, B, C, D,  0,  6, 0xF4292244);
	P(D, A, B, C,  7, 10, 0x432AFF97);
	P(C, D, A, B, 14, 15, 0xAB9423A7);
	P(B, C, D, A,  5, 21, 0xFC93A039);
	P(A, B, C, D, 12,  6, 0x655B59C3);
	P(D, A, B, C,  3, 10, 0x8F0CCC92);
	P(C, D, A, B, 10, 15, 0xFFEFF47D);
	P(B, C, D, A,  1, 21, 0x85845DD1);
	P(A, B, C, D,  8,  6, 0x6FA87E4F);
	P(D, A, B, C, 15, 10, 0xFE2CE6E0);
	P(C, D, A, B,  6, 15, 0xA3014314);
	P(B, C, D, A, 13, 21, 0x4E0811A1);
	P(A, B, C, D,  4,  6, 0xF7537E82);
	P(D, A, B, C, 11, 10, 0xBD3AF235);
	P(C, D, A, B,  2, 15, 0x2AD7D2BB);
	P(B, C, D, A,  9, 21, 0xEB86D391);

#undef F

	ctx->A += A;
	ctx->B += B;
	ctx->C += C;
	ctx->D += D;
}

static void md5_update(md_context *ctx, const uint8_t *input, uint32_t length)
{
	uint32_t left, fill;

	if (!length)
		return;

	left = ctx->totalN & 0x3F;
	fill = CSUM_CHUNK - left;

	ctx->totalN += length;
	ctx->totalN &= 0xFFFFFFFF;

	if (ctx->totalN < length)
		ctx->totalN2++;

	if (left && length >= fill) {
		memcpy(ctx->buffer + left, input, fill);
		md5_process(ctx, ctx->buffer);
		length -= fill;
		input  += fill;
		left = 0;
	}

	while (length >= CSUM_CHUNK) {
		md5_process(ctx, input);
		length -= CSUM_CHUNK;
		input  += CSUM_CHUNK;
	}

	if (length)
		memcpy(ctx->buffer + left, input, length);
}

static uint8_t md5_padding[CSUM_CHUNK] = { 0x80 };

static void md5_result(md_context *ctx, uint8_t digest[MD5_DIGEST_LEN])
{
	uint32_t last, padn;
	uint32_t high, low;
	uint8_t msglen[8];

	high = (ctx->totalN >> 29)
	     | (ctx->totalN2 <<  3);
	low  = (ctx->totalN <<  3);

	SIVAL(msglen, 0, low);
	SIVAL(msglen, 4, high);

	last = ctx->totalN & 0x3F;
	padn = last < 56 ? 56 - last : 120 - last;

	md5_update(ctx, md5_padding, padn);
	md5_update(ctx, msglen, 8);

	SIVAL(digest, 0, ctx->A);
	SIVAL(digest, 4, ctx->B);
	SIVAL(digest, 8, ctx->C);
	SIVAL(digest, 12, ctx->D);
}


static void libevil_md5sum_fd (int fd, uint8_t *out)
{
        int        i = 0;
	md_context ctx;
	uint8_t    buf[1024];

        md5_begin(&ctx);

        while ((i = read (fd, buf, sizeof buf)) > 0)
                md5_update(&ctx, buf, i);

        md5_result(&ctx, out);
}


static int libevil_md5sum_file (int dirfd, const char *path, uint8_t *out)
{
        int   fd = -1;

        fd = openat (dirfd, path, O_RDONLY, 0);

        if (fd < 0)
                return -errno;

        libevil_md5sum_fd (fd, out);

        close (fd);
        return 0;
}
