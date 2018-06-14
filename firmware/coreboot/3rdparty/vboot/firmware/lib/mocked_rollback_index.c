/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for querying, manipulating and locking rollback indices
 * stored in the TPM NVRAM.
 */

#include "sysincludes.h"
#include "utility.h"

#include "rollback_index.h"

#include "tss_constants.h"


uint32_t SetVirtualDevMode(int val)
{
	return TPM_SUCCESS;
}

uint32_t TPMClearAndReenable(void)
{
	return TPM_SUCCESS;
}

uint32_t RollbackKernelRead(uint32_t *version)
{
	*version = 0;
	return TPM_SUCCESS;
}

uint32_t RollbackKernelWrite(uint32_t version)
{
	return TPM_SUCCESS;
}

uint32_t RollbackKernelLock(int recovery_mode)
{
	return TPM_SUCCESS;
}

uint32_t RollbackFwmpRead(struct RollbackSpaceFwmp *fwmp)
{
	memset(fwmp, 0, sizeof(*fwmp));
	return TPM_SUCCESS;
}
