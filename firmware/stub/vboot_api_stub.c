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

static enum VbEcBootMode_t vboot_mode;

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

int VbExTrustEC(int devidx)
{
	return 1;
}

VbError_t VbExEcRunningRW(int devidx, int *in_rw)
{
	*in_rw = 0;
	return VBERROR_SUCCESS;
}

VbError_t VbExEcJumpToRW(int devidx)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExEcRebootToRO(int devidx)
{
	/* Nothing to reboot, so all we can do is return failure. */
	return VBERROR_UNKNOWN;
}

VbError_t VbExEcDisableJump(int devidx)
{
	return VBERROR_SUCCESS;
}

#define SHA256_HASH_SIZE 32

VbError_t VbExEcHashRW(int devidx, const uint8_t **hash, int *hash_size)
{
	static const uint8_t fake_hash[32] = {1, 2, 3, 4};

	*hash = fake_hash;
	*hash_size = sizeof(fake_hash);
	return VBERROR_SUCCESS;
}

VbError_t VbExEcGetExpectedRW(int devidx, enum VbSelectFirmware_t select,
                              const uint8_t **image, int *image_size)
{
	static uint8_t fake_image[64] = {5, 6, 7, 8};
	*image = fake_image;
	*image_size = sizeof(fake_image);
	return VBERROR_SUCCESS;
}

VbError_t VbExEcGetExpectedRWHash(int devidx, enum VbSelectFirmware_t select,
				  const uint8_t **hash, int *hash_size)
{
	static const uint8_t fake_hash[32] = {1, 2, 3, 4};

	*hash = fake_hash;
	*hash_size = sizeof(fake_hash);
	return VBERROR_SUCCESS;
}

VbError_t VbExEcUpdateRW(int devidx, const uint8_t *image, int image_size)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExEcProtectRW(int devidx)
{
	return VBERROR_SUCCESS;
}

VbError_t VbExEcEnteringMode(int devidx, enum VbEcBootMode_t mode)
{
	vboot_mode = mode;
	return VBERROR_SUCCESS;
}

enum VbEcBootMode_t VbGetMode(void)
{
	return vboot_mode;
}

int VbExLegacy(void)
{
	return 1;
}
