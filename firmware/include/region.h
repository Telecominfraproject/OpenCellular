/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Access to portions of the firmware image, perhaps later to be expanded
 * to other devices.
 */

#ifndef VBOOT_REFERENCE_REGION_H_
#define VBOOT_REFERENCE_REGION_H_

#include "bmpblk_header.h"
#include "gbb_header.h"
#include "vboot_api.h"
#include "vboot_struct.h"

/* The maximum length of a hardware ID */
#define VB_REGION_HWID_LEN	256

/**
 * Read data from a region
 *
 * @param cparams	Vboot common parameters
 * @param region	Region number to read
 * @param offset	Offset within region to start reading
 * @param size		Size of data to read
 * @param buf		Buffer to put the data into
 * @return VBERROR_... error, VBERROR_SUCCESS on success,
 */
VbError_t VbRegionReadData(VbCommonParams *cparams,
			   enum vb_firmware_region region, uint32_t offset,
			   uint32_t size, void *buf);

/**
 * Check the version of the GBB and print debug information if valid
 *
 * @param cparams	Vboot common parameters
 */
void VbRegionCheckVersion(VbCommonParams *cparams);

/**
 * Read the hardware ID from the GBB
 *
 * @param cparams	Vboot common parameters
 * @param hwid		Place to put HWID, which will be null-terminated
 * @param max_size	Maximum size of HWID including terminated null
 *			character (suggest VB_REGION_HWID_LEN). If this size
 *			it too small then VBERROR_INVALID_PARAMETER is
 *			returned.
 * @return VBERROR_... error, VBERROR_SUCCESS on success,
 */
VbError_t VbRegionReadHWID(VbCommonParams *cparams, char *hwid,
			   uint32_t max_size);

#endif  /* VBOOT_REFERENCE_REGION_H_ */
