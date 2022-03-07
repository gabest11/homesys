/*

    This file is part of libdvbcsa.

    libdvbcsa is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation; either version 2 of the License,
    or (at your option) any later version.

    libdvbcsa is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with libdvbcsa; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
    02111-1307 USA

    (c) 2006-2008 Alexandre Becoulet <alexandre.becoulet@free.fr>

*/

#ifndef DVBCSA_PV_H_
# define DVBCSA_PV_H_

#include "config.h"
#include <stdint.h>

#if !defined(DVBCSA_DEBUG) && defined(__GNUC__)
#define DVBCSA_INLINE __attribute__ ((always_inline))
#else
#define DVBCSA_INLINE __forceinline
#endif

void worddump (const char *str, const void *data, size_t len, size_t ws);

#define DVBCSA_DATA_SIZE	8
#define DVBCSA_KEYSBUFF_SIZE	56

typedef uint8_t			dvbcsa_block_t[DVBCSA_DATA_SIZE];
typedef uint8_t			dvbcsa_keys_t[DVBCSA_KEYSBUFF_SIZE];

struct dvbcsa_key_s
{
  dvbcsa_cw_t		cw;
  dvbcsa_cw_t		cws;	/* nibble swapped CW */
  dvbcsa_keys_t		sch;
};

extern const uint8_t dvbcsa_block_sbox[256];

void dvbcsa_block_decrypt (const dvbcsa_keys_t key, const dvbcsa_block_t in, dvbcsa_block_t out);
void dvbcsa_block_encrypt (const dvbcsa_keys_t key, const dvbcsa_block_t in, dvbcsa_block_t out);

void dvbcsa_stream_xor (const dvbcsa_cw_t cw, const dvbcsa_block_t iv,
			uint8_t *stream, unsigned int len);

DVBCSA_INLINE static void
dvbcsa_xor_64 (uint8_t *b, const uint8_t *a)
{
  *(uint64_t*)b ^= *(uint64_t*)a;
}

DVBCSA_INLINE static uint32_t
dvbcsa_load_le32(const uint8_t *p)
{
  return *(uint32_t*)p;
}

DVBCSA_INLINE static uint64_t
dvbcsa_load_le64(const uint8_t *p)
{
  return *(uint64_t*)p;
}

DVBCSA_INLINE static void
dvbcsa_store_le32(uint8_t *p, const uint32_t w)
{
  *(uint32_t*)p = w;
}


#endif

