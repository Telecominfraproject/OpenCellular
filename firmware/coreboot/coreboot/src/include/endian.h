/*
 * Copyright (C) 2013 The Chromium OS Authors. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but without any warranty; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _ENDIAN_H_
#define _ENDIAN_H_

#include <arch/byteorder.h>
#include <stdint.h>
#include <swab.h>

#if defined(__LITTLE_ENDIAN)
	#define cpu_to_le64(x) ((uint64_t)(x))
	#define le64_to_cpu(x) ((uint64_t)(x))
	#define cpu_to_le32(x) ((uint32_t)(x))
	#define le32_to_cpu(x) ((uint32_t)(x))
	#define cpu_to_le16(x) ((uint16_t)(x))
	#define le16_to_cpu(x) ((uint16_t)(x))
	#define cpu_to_be64(x) swab64(x)
	#define be64_to_cpu(x) swab64(x)
	#define cpu_to_be32(x) swab32(x)
	#define be32_to_cpu(x) swab32(x)
	#define cpu_to_be16(x) swab16(x)
	#define be16_to_cpu(x) swab16(x)
#elif defined(__BIG_ENDIAN)
	#define cpu_to_le64(x) swab64(x)
	#define le64_to_cpu(x) swab64(x)
	#define cpu_to_le32(x) swab32(x)
	#define le32_to_cpu(x) swab32(x)
	#define cpu_to_le16(x) swab16(x)
	#define le16_to_cpu(x) swab16(x)
	#define cpu_to_be64(x) ((uint64_t)(x))
	#define be64_to_cpu(x) ((uint64_t)(x))
	#define cpu_to_be32(x) ((uint32_t)(x))
	#define be32_to_cpu(x) ((uint32_t)(x))
	#define cpu_to_be16(x) ((uint16_t)(x))
	#define be16_to_cpu(x) ((uint16_t)(x))
#else
	#error "<arch/byteorder.h> must #define __LITTLE_ENDIAN or __BIG_ENDIAN"
#endif

#define ntohll(x) be64_to_cpu(x)
#define htonll(x) cpu_to_be64(x)
#define ntohl(x)  be32_to_cpu(x)
#define htonl(x)  cpu_to_be32(x)

#define __clrsetbits(endian, bits, addr, clear, set) \
	write##bits(addr, cpu_to_##endian##bits((endian##bits##_to_cpu( \
		read##bits(addr)) & ~((uint##bits##_t)(clear))) | (set)))

#define clrbits_le64(addr, clear)	__clrsetbits(le, 64, addr, clear, 0)
#define clrbits_be64(addr, clear)	__clrsetbits(be, 64, addr, clear, 0)
#define clrbits_le32(addr, clear)	__clrsetbits(le, 32, addr, clear, 0)
#define clrbits_be32(addr, clear)	__clrsetbits(be, 32, addr, clear, 0)
#define clrbits_le16(addr, clear)	__clrsetbits(le, 16, addr, clear, 0)
#define clrbits_be16(addr, clear)	__clrsetbits(be, 16, addr, clear, 0)

#define setbits_le64(addr, set)		__clrsetbits(le, 64, addr, 0, set)
#define setbits_be64(addr, set)		__clrsetbits(be, 64, addr, 0, set)
#define setbits_le32(addr, set)		__clrsetbits(le, 32, addr, 0, set)
#define setbits_be32(addr, set)		__clrsetbits(be, 32, addr, 0, set)
#define setbits_le16(addr, set)		__clrsetbits(le, 16, addr, 0, set)
#define setbits_be16(addr, set)		__clrsetbits(be, 16, addr, 0, set)

#define clrsetbits_le64(addr, clear, set) __clrsetbits(le, 64, addr, clear, set)
#define clrsetbits_be64(addr, clear, set) __clrsetbits(be, 64, addr, clear, set)
#define clrsetbits_le32(addr, clear, set) __clrsetbits(le, 32, addr, clear, set)
#define clrsetbits_be32(addr, clear, set) __clrsetbits(be, 32, addr, clear, set)
#define clrsetbits_le16(addr, clear, set) __clrsetbits(le, 16, addr, clear, set)
#define clrsetbits_be16(addr, clear, set) __clrsetbits(be, 16, addr, clear, set)

#define clrsetbits_8(addr, clear, set) \
	write8(addr, (read8(addr) & ~(clear)) | (set))
#define clrbits_8(addr, clear) clrsetbits_8(addr, clear, 0)
#define setbits_8(addr, set) setbits_8(addr, 0, set)

#ifndef __ROMCC__
/*
 * Portable (API) endian support that can be used in code that is shared
 * with userspace (man 3 endian) tools.
 */
static inline uint16_t htobe16(uint16_t host_16bits)
{
	return cpu_to_be16(host_16bits);
}

static inline uint16_t htole16(uint16_t host_16bits)
{
	return cpu_to_le16(host_16bits);
}

static inline uint16_t be16toh(uint16_t big_endian_16bits)
{
	return be16_to_cpu(big_endian_16bits);
}

static inline uint16_t le16toh(uint16_t little_endian_16bits)
{
	return le16_to_cpu(little_endian_16bits);
}

static inline uint32_t htobe32(uint32_t host_32bits)
{
	return cpu_to_be32(host_32bits);
}

static inline uint32_t htole32(uint32_t host_32bits)
{
	return cpu_to_le32(host_32bits);
}

static inline uint32_t be32toh(uint32_t big_endian_32bits)
{
	return be32_to_cpu(big_endian_32bits);
}

static inline uint32_t le32toh(uint32_t little_endian_32bits)
{
	return le32_to_cpu(little_endian_32bits);
}

static inline uint64_t htobe64(uint64_t host_64bits)
{
	return cpu_to_be64(host_64bits);
}

static inline uint64_t htole64(uint64_t host_64bits)
{
	return cpu_to_le64(host_64bits);
}

static inline uint64_t be64toh(uint64_t big_endian_64bits)
{
	return be64_to_cpu(big_endian_64bits);
}

static inline uint64_t le64toh(uint64_t little_endian_64bits)
{
	return le64_to_cpu(little_endian_64bits);
}
#endif

#endif
