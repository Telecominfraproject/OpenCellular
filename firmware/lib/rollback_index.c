/* Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Functions for querying, manipulating and locking rollback indices
 * stored in the TPM NVRAM.
 */

#include "sysincludes.h"

#include "crc8.h"
#include "rollback_index.h"
#include "tlcl.h"
#include "tss_constants.h"
#include "utility.h"
#include "vboot_api.h"

#ifndef offsetof
#define offsetof(A,B) __builtin_offsetof(A,B)
#endif

/*
 * Provide protoypes for functions not in the header file. These prototypes
 * fix -Wmissing-prototypes warnings.
 */
uint32_t ReadSpaceFirmware(RollbackSpaceFirmware *rsf);
uint32_t WriteSpaceFirmware(RollbackSpaceFirmware *rsf);
uint32_t ReadSpaceKernel(RollbackSpaceKernel *rsk);
uint32_t WriteSpaceKernel(RollbackSpaceKernel *rsk);

#ifdef FOR_TEST
/*
 * Compiling for unit test, so we need the real implementations of
 * rollback functions.  The unit test mocks the underlying tlcl
 * functions, so this is ok to run on the host.
 */
#undef CHROMEOS_ENVIRONMENT
#undef DISABLE_ROLLBACK_TPM
#endif

#define RETURN_ON_FAILURE(tpm_command) do {				\
		uint32_t result_;					\
		if ((result_ = (tpm_command)) != TPM_SUCCESS) {		\
			VBDEBUG(("Rollback: %08x returned by " #tpm_command \
				 "\n", (int)result_));			\
			return result_;					\
		}							\
	} while (0)


uint32_t TPMClearAndReenable(void)
{
	VBDEBUG(("TPM: Clear and re-enable\n"));
	RETURN_ON_FAILURE(TlclForceClear());
	RETURN_ON_FAILURE(TlclSetEnable());
	RETURN_ON_FAILURE(TlclSetDeactivated(0));

	return TPM_SUCCESS;
}

uint32_t SafeWrite(uint32_t index, const void *data, uint32_t length)
{
	uint32_t result = TlclWrite(index, data, length);
	if (result == TPM_E_MAXNVWRITES) {
		RETURN_ON_FAILURE(TPMClearAndReenable());
		return TlclWrite(index, data, length);
	} else {
		return result;
	}
}

/* Functions to read and write firmware and kernel spaces. */
uint32_t ReadSpaceFirmware(RollbackSpaceFirmware *rsf)
{
	uint32_t r;
	int attempts = 3;

	while (attempts--) {
		r = TlclRead(FIRMWARE_NV_INDEX, rsf,
			     sizeof(RollbackSpaceFirmware));
		if (r != TPM_SUCCESS)
			return r;

		/*
		 * No CRC in this version, so we'll create one when we write
		 * it. Note that we're marking this as version 2, not
		 * ROLLBACK_SPACE_FIRMWARE_VERSION, because version 2 just
		 * added the CRC. Later versions will need to set default
		 * values for any extra fields explicitly (probably here).
		 */
		if (rsf->struct_version < 2) {
			/* Danger Will Robinson! Danger! */
			rsf->struct_version = 2;
			return TPM_SUCCESS;
		}

		/*
		 * If the CRC is good, we're done. If it's bad, try a couple
		 * more times to see if it gets better before we give up. It
		 * could just be noise.
		 */
		if (rsf->crc8 == Crc8(rsf,
				      offsetof(RollbackSpaceFirmware, crc8)))
			return TPM_SUCCESS;

		VBDEBUG(("TPM: %s() - bad CRC\n", __func__));
	}

	VBDEBUG(("TPM: %s() - too many bad CRCs, giving up\n", __func__));
	return TPM_E_CORRUPTED_STATE;
}

uint32_t WriteSpaceFirmware(RollbackSpaceFirmware *rsf)
{
	RollbackSpaceFirmware rsf2;
	uint32_t r;
	int attempts = 3;

	/* All writes should use struct_version 2 or greater. */
	if (rsf->struct_version < 2)
		rsf->struct_version = 2;
	rsf->crc8 = Crc8(rsf, offsetof(RollbackSpaceFirmware, crc8));

	while (attempts--) {
		r = SafeWrite(FIRMWARE_NV_INDEX, rsf,
			      sizeof(RollbackSpaceFirmware));
		/* Can't write, not gonna try again */
		if (r != TPM_SUCCESS)
			return r;

		/* Read it back to be sure it got the right values. */
		r = ReadSpaceFirmware(&rsf2);    /* This checks the CRC */
		if (r == TPM_SUCCESS)
			return r;

		VBDEBUG(("TPM: %s() - bad CRC\n", __func__));
		/* Try writing it again. Maybe it was garbled on the way out. */
	}

	VBDEBUG(("TPM: %s() - too many bad CRCs, giving up\n", __func__));
	return TPM_E_CORRUPTED_STATE;
}

uint32_t SetVirtualDevMode(int val)
{
	RollbackSpaceFirmware rsf;

	VBDEBUG(("TPM: Entering %s()\n", __func__));
	if (TPM_SUCCESS != ReadSpaceFirmware(&rsf))
		return VBERROR_TPM_FIRMWARE_SETUP;

	VBDEBUG(("TPM: flags were 0x%02x\n", rsf.flags));
	if (val)
		rsf.flags |= FLAG_VIRTUAL_DEV_MODE_ON;
	else
		rsf.flags &= ~FLAG_VIRTUAL_DEV_MODE_ON;
	/*
	 * NOTE: This doesn't update the FLAG_LAST_BOOT_DEVELOPER bit.  That
	 * will be done on the next boot.
	 */
	VBDEBUG(("TPM: flags are now 0x%02x\n", rsf.flags));

	if (TPM_SUCCESS != WriteSpaceFirmware(&rsf))
		return VBERROR_TPM_SET_BOOT_MODE_STATE;

	VBDEBUG(("TPM: Leaving %s()\n", __func__));
	return VBERROR_SUCCESS;
}

uint32_t ReadSpaceKernel(RollbackSpaceKernel *rsk)
{
	uint32_t r;
	int attempts = 3;

	while (attempts--) {
		r = TlclRead(KERNEL_NV_INDEX, rsk, sizeof(RollbackSpaceKernel));
		if (r != TPM_SUCCESS)
			return r;

		/*
		 * No CRC in this version, so we'll create one when we write
		 * it. Note that we're marking this as version 2, not
		 * ROLLBACK_SPACE_KERNEL_VERSION, because version 2 just added
		 * the CRC. Later versions will need to set default values for
		 * any extra fields explicitly (probably here).
		 */
		if (rsk->struct_version < 2) {
			/* Danger Will Robinson! Danger! */
			rsk->struct_version = 2;
			return TPM_SUCCESS;
		}

		/*
		 * If the CRC is good, we're done. If it's bad, try a couple
		 * more times to see if it gets better before we give up. It
		 * could just be noise.
		 */
		if (rsk->crc8 == Crc8(rsk, offsetof(RollbackSpaceKernel, crc8)))
			return TPM_SUCCESS;

		VBDEBUG(("TPM: %s() - bad CRC\n", __func__));
	}

	VBDEBUG(("TPM: %s() - too many bad CRCs, giving up\n", __func__));
	return TPM_E_CORRUPTED_STATE;
}

uint32_t WriteSpaceKernel(RollbackSpaceKernel *rsk)
{
	RollbackSpaceKernel rsk2;
	uint32_t r;
	int attempts = 3;

	/* All writes should use struct_version 2 or greater. */
	if (rsk->struct_version < 2)
		rsk->struct_version = 2;
	rsk->crc8 = Crc8(rsk, offsetof(RollbackSpaceKernel, crc8));

	while (attempts--) {
		r = SafeWrite(KERNEL_NV_INDEX, rsk,
			      sizeof(RollbackSpaceKernel));
		/* Can't write, not gonna try again */
		if (r != TPM_SUCCESS)
			return r;

		/* Read it back to be sure it got the right values. */
		r = ReadSpaceKernel(&rsk2);    /* This checks the CRC */
		if (r == TPM_SUCCESS)
			return r;

		VBDEBUG(("TPM: %s() - bad CRC\n", __func__));
		/* Try writing it again. Maybe it was garbled on the way out. */
	}

	VBDEBUG(("TPM: %s() - too many bad CRCs, giving up\n", __func__));
	return TPM_E_CORRUPTED_STATE;
}

#ifdef DISABLE_ROLLBACK_TPM
/* Dummy implementations which don't support TPM rollback protection */

uint32_t RollbackKernelRead(uint32_t* version)
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
	Memset(fwmp, 0, sizeof(*fwmp));
	return TPM_SUCCESS;
}

#else

uint32_t RollbackKernelRead(uint32_t* version)
{
	RollbackSpaceKernel rsk;

	/*
	 * Read the kernel space and verify its permissions.  If the kernel
	 * space has the wrong permission, or it doesn't contain the right
	 * identifier, we give up.  This will need to be fixed by the
	 * recovery kernel.  We have to worry about this because at any time
	 * (even with PP turned off) the TPM owner can remove and redefine a
	 * PP-protected space (but not write to it).
	 */
	RETURN_ON_FAILURE(ReadSpaceKernel(&rsk));
#ifndef TPM2_MODE
	/*
	 * TODO(vbendeb): restore this when it is defined how the kernel space
	 * gets protected.
	 */
	{
		uint32_t perms, uid;

		RETURN_ON_FAILURE(TlclGetPermissions(KERNEL_NV_INDEX, &perms));
		Memcpy(&uid, &rsk.uid, sizeof(uid));
		if (TPM_NV_PER_PPWRITE != perms ||
		    ROLLBACK_SPACE_KERNEL_UID != uid)
			return TPM_E_CORRUPTED_STATE;
	}
#endif
	Memcpy(version, &rsk.kernel_versions, sizeof(*version));
	VBDEBUG(("TPM: RollbackKernelRead %x\n", (int)*version));
	return TPM_SUCCESS;
}

uint32_t RollbackKernelWrite(uint32_t version)
{
	RollbackSpaceKernel rsk;
	uint32_t old_version;
	RETURN_ON_FAILURE(ReadSpaceKernel(&rsk));
	Memcpy(&old_version, &rsk.kernel_versions, sizeof(old_version));
	VBDEBUG(("TPM: RollbackKernelWrite %x --> %x\n",
		 (int)old_version, (int)version));
	Memcpy(&rsk.kernel_versions, &version, sizeof(version));
	return WriteSpaceKernel(&rsk);
}

uint32_t RollbackKernelLock(int recovery_mode)
{
	static int kernel_locked = 0;
	uint32_t r;

	if (recovery_mode || kernel_locked)
		return TPM_SUCCESS;

	r = TlclLockPhysicalPresence();
	if (TPM_SUCCESS == r)
		kernel_locked = 1;
	return r;
}

uint32_t RollbackFwmpRead(struct RollbackSpaceFwmp *fwmp)
{
	union {
		/*
		 * Use a union for buf and bf, rather than making bf a pointer
		 * to a bare uint8_t[] buffer.  This ensures bf will be aligned
		 * if necesssary for the target platform.
		 */
		uint8_t buf[FWMP_NV_MAX_SIZE];
		struct RollbackSpaceFwmp bf;
	} u;
	uint32_t r;
	int attempts = 3;

	/* Clear destination in case error or FWMP not present */
	Memset(fwmp, 0, sizeof(*fwmp));

	while (attempts--) {
		/* Try to read entire 1.0 struct */
		r = TlclRead(FWMP_NV_INDEX, u.buf, sizeof(u.bf));
		if (r == TPM_E_BADINDEX) {
			/* Missing space is not an error; use defaults */
			VBDEBUG(("TPM: %s() - no FWMP space\n", __func__));
			return TPM_SUCCESS;
		} else if (r != TPM_SUCCESS) {
			VBDEBUG(("TPM: %s() - read returned 0x%x\n",
				 __func__, r));
			return r;
		}

		/*
		 * Struct must be at least big enough for 1.0, but not bigger
		 * than our buffer size.
		 */
		if (u.bf.struct_size < sizeof(u.bf) ||
		    u.bf.struct_size > sizeof(u.buf))
			return TPM_E_STRUCT_SIZE;

		/*
		 * If space is bigger than we expect, re-read so we properly
		 * compute the CRC.
		 */
		if (u.bf.struct_size > sizeof(u.bf)) {
			r = TlclRead(FWMP_NV_INDEX, u.buf, u.bf.struct_size);
			if (r != TPM_SUCCESS)
				return r;
		}

		/* Verify CRC */
		if (u.bf.crc != Crc8(u.buf + 2, u.bf.struct_size - 2)) {
			VBDEBUG(("TPM: %s() - bad CRC\n", __func__));
			continue;
		}

		/* Verify major version is compatible */
		if ((u.bf.struct_version >> 4) !=
		    (ROLLBACK_SPACE_FWMP_VERSION >> 4))
			return TPM_E_STRUCT_VERSION;

		/*
		 * Copy to destination.  Note that if the space is bigger than
		 * we expect (due to a minor version change), we only copy the
		 * part of the FWMP that we know what to do with.
		 *
		 * If this were a 1.1+ reader and the source was a 1.0 struct,
		 * we would need to take care of initializing the extra fields
		 * added in 1.1+.  But that's not an issue yet.
		 */
		Memcpy(fwmp, &u.bf, sizeof(*fwmp));
		return TPM_SUCCESS;
	}

	VBDEBUG(("TPM: %s() - too many bad CRCs, giving up\n", __func__));
	return TPM_E_CORRUPTED_STATE;
}

#endif /* DISABLE_ROLLBACK_TPM */
