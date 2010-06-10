/* Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Some TPM constants and type definitions for standalone compilation for use in
 * the firmware
 */

/* FIXME(gauravsh):
 * NOTE: This file is copied over from
 *       src/platform/tpm_lite/src/tlcl/tss_constants.h
 * Ideally, we want to directly include it without having two maintain
 * duplicate copies in sync. But in the current model, this is hard
 * to do without breaking standalone compilation.
 * Eventually tpm_lite should be moved into vboot_reference.
 */

#ifndef TPM_LITE_TSS_CONSTANTS_H_
#define TPM_LITE_TSS_CONSTANTS_H_

#include <stdint.h>

#define TPM_MAX_COMMAND_SIZE 4096
#define TPM_LARGE_ENOUGH_COMMAND_SIZE 256  /* saves space in the firmware */

#define TPM_SUCCESS ((uint32_t)0x00000000)
#define TPM_E_BADINDEX ((uint32_t)0x00000002)
#define TPM_E_MAXNVWRITES ((uint32_t)0x00000048)
#define TPM_E_ALREADY_INITIALIZED ((uint32_t)0x00005000)     /* vboot local */
#define TPM_E_INTERNAL_INCONSISTENCY ((uint32_t)0x00005001)  /* vboot local */

#define TPM_NV_INDEX0 ((uint32_t)0x00000000)
#define TPM_NV_INDEX_LOCK ((uint32_t)0xffffffff)
#define TPM_NV_PER_WRITE_STCLEAR (((uint32_t)1)<<14)
#define TPM_NV_PER_PPWRITE (((uint32_t)1)<<0)
#define TPM_NV_PER_GLOBALLOCK (((uint32_t)1)<<15)

typedef uint8_t TSS_BOOL;
typedef uint16_t TPM_STRUCTURE_TAG;

typedef struct tdTPM_WRITE_INFO {
  uint32_t nvIndex;
  uint32_t offset;
  uint32_t dataSize;
} TPM_WRITE_INFO;

typedef struct tdTPM_PERMANENT_FLAGS
{
    TPM_STRUCTURE_TAG tag;
    TSS_BOOL disable;
    TSS_BOOL ownership;
    TSS_BOOL deactivated;
    TSS_BOOL readPubek;
    TSS_BOOL disableOwnerClear;
    TSS_BOOL allowMaintenance;
    TSS_BOOL physicalPresenceLifetimeLock;
    TSS_BOOL physicalPresenceHWEnable;
    TSS_BOOL physicalPresenceCMDEnable;
    TSS_BOOL CEKPUsed;
    TSS_BOOL TPMpost;
    TSS_BOOL TPMpostLock;
    TSS_BOOL FIPS;
    TSS_BOOL Operator;
    TSS_BOOL enableRevokeEK;
    TSS_BOOL nvLocked;
    TSS_BOOL readSRKPub;
    TSS_BOOL tpmEstablished;
    TSS_BOOL maintenanceDone;
    TSS_BOOL disableFullDALogicInfo;
} TPM_PERMANENT_FLAGS;

#define TPM_ALL_LOCALITIES (TPM_LOC_ZERO | TPM_LOC_ONE | TPM_LOC_TWO \
                            | TPM_LOC_THREE | TPM_LOC_FOUR)  /* 0x1f */

#define TPM_ENCAUTH_SIZE 20
#define TPM_PUBEK_SIZE 256

#endif  /* TPM_LITE_TSS_CONSTANTS_H_ */
