/*
 * This file is part of the coreboot project.
 *
 * Copyright 2017 Intel Corporation.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <random.h>
#include <rules.h>

/*
 * Intel recommends that applications attempt 10 retries in a tight loop
 * in the unlikely event that the RDRAND instruction does not successfully
 * return a random number. The odds of ten failures in a row would in fact
 * be an indication of a larger CPU issue.
 */
#define RDRAND_RETRY_LOOPS 10

/*
 * Generate a 32-bit random number through RDRAND instruction.
 * Carry flag is set on RDRAND success and 0 on failure.
 */
static inline uint8_t rdrand_32(uint32_t *rand)
{
	uint8_t carry;

	__asm__ __volatile__(
	     ".byte 0x0f; .byte 0xc7; .byte 0xf0; setc %1"
	     : "=a" (*rand), "=qm" (carry));
	return carry;
}

#if ENV_X86_64
/*
 * Generate a 64-bit random number through RDRAND instruction.
 * Carry flag is set on RDRAND success and 0 on failure.
 */
static inline uint8_t rdrand_64(uint64_t *rand)
{
	uint8_t carry;

	__asm__ __volatile__(
	     ".byte 0x48; .byte 0x0f; .byte 0xc7; .byte 0xf0; setc %1"
	     : "=a" (*rand), "=qm" (carry));
	return carry;
}
#endif

int get_random_number_32(uint32_t *rand)
{
	int i;

	/* Perform a loop call until RDRAND succeeds or returns failure. */
	for (i = 0; i < RDRAND_RETRY_LOOPS; i++) {
		if (rdrand_32(rand))
			return 0;
	}
	return -1;
}

int get_random_number_64(uint64_t *rand)
{
	int i;
	uint32_t rand_high, rand_low;

	/* Perform a loop call until RDRAND succeeds or returns failure. */
	for (i = 0; i < RDRAND_RETRY_LOOPS; i++) {
#if ENV_X86_64
		if (rdrand_64(rand))
			return 0;
		else
#endif
		if (rdrand_32(&rand_high) && rdrand_32(&rand_low)) {
			*rand = ((uint64_t)rand_high << 32) |
				(uint64_t)rand_low;
			return 0;
		}
	}
	return -1;
}
