/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Some TPM constants and type definitions for standalone compilation for use
 * in the firmware
 */
#ifndef __VBOOT_REFERENCE_FIRMWARE_INCLUDE_TPM1_TSS_CONSTANTS_H
#define __VBOOT_REFERENCE_FIRMWARE_INCLUDE_TPM1_TSS_CONSTANTS_H

#include <stdint.h>

#define TPM_MAX_COMMAND_SIZE 4096
#define TPM_LARGE_ENOUGH_COMMAND_SIZE 256  /* saves space in the firmware */
#define TPM_PUBEK_SIZE 256
#define TPM_PCR_DIGEST 20

#define TPM_NV_INDEX0            ((uint32_t) 0x00000000)
#define TPM_NV_INDEX_LOCK        ((uint32_t) 0xffffffff)
#define TPM_NV_PER_GLOBALLOCK    (((uint32_t) 1) << 15)
#define TPM_NV_PER_PPWRITE       (((uint32_t) 1) << 0)
#define TPM_NV_PER_READ_STCLEAR  (((uint32_t)1) << 31)
#define TPM_NV_PER_WRITE_STCLEAR (((uint32_t)1) << 14)

#define TPM_TAG_NV_ATTRIBUTES          ((uint16_t) 0x0017)
#define TPM_TAG_NV_DATA_PUBLIC         ((uint16_t) 0x0018)
#define TPM_TAG_KEY12                  ((uint16_t) 0x0028)

#define TPM_TAG_RQU_COMMAND       ((uint16_t) 0xc1)
#define TPM_TAG_RQU_AUTH1_COMMAND ((uint16_t) 0xc2)
#define TPM_TAG_RQU_AUTH2_COMMAND ((uint16_t) 0xc3)

#define TPM_TAG_RSP_COMMAND       ((uint16_t) 0xc4)
#define TPM_TAG_RSP_AUTH1_COMMAND ((uint16_t) 0xc5)
#define TPM_TAG_RSP_AUTH2_COMMAND ((uint16_t) 0xc6)

typedef uint8_t TSS_BOOL;
typedef uint8_t TPM_BOOL;
typedef uint16_t TPM_TAG;
typedef uint16_t TPM_STRUCTURE_TAG;
typedef uint32_t TPM_NV_INDEX;
typedef uint32_t TPM_NV_PER_ATTRIBUTES;
typedef uint8_t TPM_LOCALITY_SELECTION;
typedef uint32_t TPM_COMMAND_CODE;
typedef uint16_t TPM_PHYSICAL_PRESENCE;
typedef uint16_t TPM_STARTUP_TYPE;
typedef uint32_t TPM_CAPABILITY_AREA;

#define TPM_CAP_FLAG           ((uint32_t) 0x00000004)
#define TPM_CAP_FLAG_PERMANENT ((uint32_t) 0x00000108)
#define TPM_CAP_FLAG_VOLATILE  ((uint32_t) 0x00000109)

#define TPM_CAP_PROPERTY        ((uint32_t) 0x00000005)
#define TPM_CAP_PROP_OWNER      ((uint32_t) 0x00000111)
#define TPM_CAP_NV_INDEX        ((uint32_t) 0x00000011)
#define TPM_CAP_GET_VERSION_VAL ((uint32_t) 0x0000001a)

#define TPM_AUTH_ALWAYS  ((uint8_t) 0x01)

#define TPM_KEY_USAGE_STORAGE ((uint16_t) 0x0011)

#define TPM_ALG_RSA ((uint16_t)0x0001)

#define TPM_ES_RSAESOAEP_SHA1_MGF1 ((uint16_t)0x0003)

#define TPM_SS_NONE ((uint16_t)0x0001)

#define TPM_PID_OWNER ((uint16_t) 0x0005)

#define TPM_ST_CLEAR       ((uint16_t) 0x0001)
#define TPM_ST_STATE       ((uint16_t) 0x0002)
#define TPM_ST_DEACTIVATED ((uint16_t) 0x0003)

#define TPM_LOC_FOUR   (((uint32_t)1)<<4)
#define TPM_LOC_THREE  (((uint32_t)1)<<3)
#define TPM_LOC_TWO    (((uint32_t)1)<<2)
#define TPM_LOC_ONE    (((uint32_t)1)<<1)
#define TPM_LOC_ZERO   (((uint32_t)1)<<0)

#define TPM_PHYSICAL_PRESENCE_LOCK          ((uint16_t) 0x0004)
#define TPM_PHYSICAL_PRESENCE_PRESENT       ((uint16_t) 0x0008)
#define TPM_PHYSICAL_PRESENCE_NOTPRESENT    ((uint16_t) 0x0010)
#define TPM_PHYSICAL_PRESENCE_CMD_ENABLE    ((uint16_t) 0x0020)
#define TPM_PHYSICAL_PRESENCE_HW_ENABLE     ((uint16_t) 0x0040)
#define TPM_PHYSICAL_PRESENCE_LIFETIME_LOCK ((uint16_t) 0x0080)
#define TPM_PHYSICAL_PRESENCE_CMD_DISABLE   ((uint16_t) 0x0100)
#define TPM_PHYSICAL_PRESENCE_HW_DISABLE    ((uint16_t) 0x0200)

#define TPM_SHA1_160_HASH_LEN    0x14
#define TPM_SHA1BASED_NONCE_LEN  TPM_SHA1_160_HASH_LEN

#define TPM_AUTH_DATA_LEN    0x14

#define TPM_RSA_2048_LEN    0x100

typedef struct tdTPM_DIGEST
{
    uint8_t  digest[TPM_SHA1_160_HASH_LEN];
} TPM_DIGEST;

typedef TPM_DIGEST TPM_COMPOSITE_HASH;

typedef struct tdTPM_PCR_SELECTION
{
    uint16_t  sizeOfSelect;
    uint8_t   *pcrSelect;
} TPM_PCR_SELECTION;

typedef struct tdTPM_NV_ATTRIBUTES
{
    TPM_STRUCTURE_TAG     tag;
    TPM_NV_PER_ATTRIBUTES attributes;
} TPM_NV_ATTRIBUTES;

typedef struct tdTPM_PCR_INFO_SHORT
{
    TPM_PCR_SELECTION      pcrSelection;
    TPM_LOCALITY_SELECTION localityAtRelease;
    TPM_COMPOSITE_HASH     digestAtRelease;
}  TPM_PCR_INFO_SHORT;

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

typedef struct tdTPM_STCLEAR_FLAGS{
	TPM_STRUCTURE_TAG tag;
	TSS_BOOL deactivated;
	TSS_BOOL disableForceClear;
	TSS_BOOL physicalPresence;
	TSS_BOOL physicalPresenceLock;
	TSS_BOOL bGlobalLock;
} TPM_STCLEAR_FLAGS;

typedef struct tdTPM_NV_DATA_PUBLIC
{
    TPM_STRUCTURE_TAG  tag;
    TPM_NV_INDEX       nvIndex;
    TPM_PCR_INFO_SHORT pcrInfoRead;
    TPM_PCR_INFO_SHORT pcrInfoWrite;
    TPM_NV_ATTRIBUTES  permission;
    TPM_BOOL           bReadSTClear;
    TPM_BOOL           bWriteSTClear;
    TPM_BOOL           bWriteDefine;
    uint32_t           dataSize;
} TPM_NV_DATA_PUBLIC;

typedef struct tdTPM_NONCE
{
    uint8_t nonce[TPM_SHA1BASED_NONCE_LEN];
} TPM_NONCE;

typedef struct tdTPM_IFX_FIRMWAREPACKAGE {
    uint32_t FwPackageIdentifier;
    uint32_t Version;
    uint32_t StaleVersion;
} TPM_IFX_FIRMWAREPACKAGE;

typedef struct tdTPM_IFX_FIELDUPGRADEINFO
{
    uint16_t wMaxDataSize;
    TPM_IFX_FIRMWAREPACKAGE sBootloaderFirmwarePackage;
    TPM_IFX_FIRMWAREPACKAGE sFirmwarePackages[2];
    uint16_t wSecurityModuleStatus;
    TPM_IFX_FIRMWAREPACKAGE sProcessFirmwarePackage;
    uint16_t wFieldUpgradeCounter;
} TPM_IFX_FIELDUPGRADEINFO;

#define TPM_IFX_FieldUpgradeInfoRequest2  ((uint8_t) 0x11)

/* Ordinals */

#define TPM_ORD_ContinueSelfTest  ((uint32_t) 0x00000053)
#define TPM_ORD_Extend            ((uint32_t) 0x00000014)
#define TPM_ORD_FieldUpgrade      ((uint32_t) 0x000000AA)
#define TPM_ORD_ForceClear        ((uint32_t) 0x0000005D)
#define TPM_ORD_GetCapability     ((uint32_t) 0x00000065)
#define TPM_ORD_GetRandom                         ((uint32_t) 0x00000046)
#define TPM_ORD_NV_DefineSpace    ((uint32_t) 0x000000CC)
#define TPM_ORD_NV_ReadValue      ((uint32_t) 0x000000CF)
#define TPM_ORD_NV_WriteValue     ((uint32_t) 0x000000CD)
#define TPM_ORD_OIAP              ((uint32_t) 0x0000000A)
#define TPM_ORD_PcrRead           ((uint32_t) 0x00000015)
#define TPM_ORD_PhysicalEnable    ((uint32_t) 0x0000006F)
#define TPM_ORD_PhysicalDisable   ((uint32_t) 0x00000070)
#define TSC_ORD_PhysicalPresence  ((uint32_t) 0x4000000A)
#define TPM_ORD_PhysicalSetDeactivated    ((uint32_t) 0x00000072)
#define TPM_ORD_ReadPubek         ((uint32_t) 0x0000007C)
#define TPM_ORD_SaveState         ((uint32_t) 0x00000098)
#define TPM_ORD_SelfTestFull      ((uint32_t) 0x00000050)
#define TPM_ORD_Startup           ((uint32_t) 0x00000099)
#define TPM_ORD_TakeOwnership     ((uint32_t) 0x0000000D)

#endif  /* ! __VBOOT_REFERENCE_FIRMWARE_INCLUDE_TPM1_TSS_CONSTANTS_H */
