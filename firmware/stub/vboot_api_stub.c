/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Stub implementations of firmware-provided API functions.
 */

#include <stdint.h>

#define _STUB_IMPLEMENTATION_

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include "vboot_api.h"

void VbExSleepMs(uint32_t msec)
{
}

VbError_t VbExBeep(uint32_t msec, uint32_t frequency)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExDisplayInit(uint32_t *width, uint32_t *height)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExDisplayBacklight(uint8_t enable)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExDisplaySetDimension(uint32_t width, uint32_t height)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExDisplayScreen(uint32_t screen_type)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExDisplayImage(uint32_t x, uint32_t y,
                           void *buffer, uint32_t buffersize)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExDisplayDebugInfo(const char *info_str)
{
	return VBERROR_SUCCESS;
}

uint32_t VbExKeyboardRead(void)
{
	return 0;
}

uint32_t VbExKeyboardReadWithFlags(uint32_t *flags_ptr)
{
	return 0;
}

uint32_t VbExGetSwitches(uint32_t mask)
{
	return 0;
}

uint32_t VbExIsShutdownRequested(void)
{
	return 0;
}

VbError_t VbExDecompress(void *inbuf, uint32_t in_size,
                         uint32_t compression_type,
                         void *outbuf, uint32_t *out_size)
{
	return VBERROR_SUCCESS;
}

int VbExTrustEC(void)
{
	return 1;
}

VbError_t VbExEcRunningRW(int *in_rw)
{
	*in_rw = 0;
	return VBERROR_SUCCESS;
}

VbError_t VbExEcJumpToRW(void)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExEcRebootToRO(void)
{
	/* Nothing to reboot, so all we can do is return failure. */
	return VBERROR_UNKNOWN;
}

VbError_t VbExEcDisableJump(void)
{
	return VBERROR_SUCCESS;
}

#define SHA256_HASH_SIZE 32

VbError_t VbExEcHashRW(const uint8_t **hash, int *hash_size)
{
	static const uint8_t fake_hash[32] = {1, 2, 3, 4};

	*hash = fake_hash;
	*hash_size = sizeof(fake_hash);
	return VBERROR_SUCCESS;
}

VbError_t VbExEcGetExpectedRW(enum VbSelectFirmware_t select,
                              const uint8_t **image, int *image_size)
{
	static uint8_t fake_image[64] = {5, 6, 7, 8};
	*image = fake_image;
	*image_size = sizeof(fake_image);
	return VBERROR_SUCCESS;
}

VbError_t VbExEcGetExpectedRWHash(enum VbSelectFirmware_t select,
				  const uint8_t **hash, int *hash_size)
{
	static const uint8_t fake_hash[32] = {1, 2, 3, 4};

	*hash = fake_hash;
	*hash_size = sizeof(fake_hash);
	return VBERROR_SUCCESS;
}

VbError_t VbExEcUpdateRW(const uint8_t *image, int image_size)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExEcProtectRW(void)
{
	return VBERROR_SUCCESS;
}

int VbExLegacy(void)
{
	return 1;
}
