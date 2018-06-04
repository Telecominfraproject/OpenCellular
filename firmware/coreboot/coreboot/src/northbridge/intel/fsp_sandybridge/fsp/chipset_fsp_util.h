/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2013-2014 Sage Electronic Engineering, LLC.
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

#ifndef CHIPSET_FSP_UTIL_H
#define CHIPSET_FSP_UTIL_H

#include <fsptypes.h>
#include <fspfv.h>
#include <fspffs.h>
#include <fspapi.h>
#include <fspplatform.h>
#include <fspinfoheader.h>
#include <fsphob.h>
#if IS_ENABLED(CONFIG_SOUTHBRIDGE_INTEL_FSP_I89XX)
#include <peifsp.h>
#include <fsp_vpd.h>
#endif

#define FSP_RESERVE_MEMORY_SIZE	0x200000

#define FSP_INFO_HEADER_GUID \
  { \
  0x912740BE, 0x2284, 0x4734, {0xB9, 0x71, 0x84, 0xB0, 0x27, 0x35, 0x3F, 0x0C} \
  }

#define FSP_NON_VOLATILE_STORAGE_HOB_GUID \
  { \
  0x721acf02, 0x4d77, 0x4c2a, { 0xb3, 0xdc, 0x27, 0xb, 0x7b, 0xa9, 0xe4, 0xb0 } \
  }


/*
 *The FSP Image ID is different for each platform's FSP and
 * can be used to verify that the right FSP binary is loaded.
 */

#if IS_ENABLED(CONFIG_SOUTHBRIDGE_INTEL_FSP_I89XX)
/* ST2-FSP0 */
#define FSP_IMAGE_ID_DWORD0 0x2D325453
#define FSP_IMAGE_ID_DWORD1 0x30505346
#elif IS_ENABLED(CONFIG_SOUTHBRIDGE_INTEL_FSP_BD82X6X)
/* CC2-FSP\0 */
#define FSP_IMAGE_ID_DWORD0 0x2D324343
#define FSP_IMAGE_ID_DWORD1 0x00505346
#endif

#ifdef __PRE_RAM__
void main(FSP_INFO_HEADER *fsp_info_header);
void romstage_main_continue(EFI_STATUS status, VOID *HobListPtr);
#endif

#endif /* CHIPSET_FSP_UTIL_H */
