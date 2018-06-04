/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2016 Kyösti Mälkki
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

#ifndef _STATE_MACHINE_H_
#define _STATE_MACHINE_H_

#include <stdint.h>
#include <AGESA.h>
#include <AMD.h>

#define HAS_LEGACY_WRAPPER IS_ENABLED(CONFIG_BINARYPI_LEGACY_WRAPPER)

/* eventlog */
const char *agesa_struct_name(int state);
const char *heap_status_name(int status);
void agesawrapper_trace(AGESA_STATUS ret, AMD_CONFIG_PARAMS *StdHeader, const char *func);
AGESA_STATUS agesawrapper_amdreadeventlog(UINT8 HeapStatus);

/* For suspend-to-ram support. */

#if !IS_ENABLED(CONFIG_CPU_AMD_PI)
/* TODO: With binaryPI we need different interface. */
AGESA_STATUS OemInitResume(AMD_S3_PARAMS *dataBlock);
AGESA_STATUS OemS3LateRestore(AMD_S3_PARAMS *dataBlock);
AGESA_STATUS OemS3Save(AMD_S3_PARAMS *dataBlock);
#endif

/* For FCH */
AGESA_STATUS fchs3earlyrestore(AMD_CONFIG_PARAMS *StdHeader);
AGESA_STATUS fchs3laterestore(AMD_CONFIG_PARAMS *StdHeader);

struct sysinfo
{
	AMD_CONFIG_PARAMS StdHeader;

	int s3resume;
};

void agesa_main(struct sysinfo *cb);
void agesa_postcar(struct sysinfo *cb);

void board_BeforeAgesa(struct sysinfo *cb);
void platform_once(struct sysinfo *cb);

void agesa_set_interface(struct sysinfo *cb);
int agesa_execute_state(struct sysinfo *cb, AGESA_STRUCT_NAME func);

/* AGESA dispatchers */

AGESA_STATUS module_dispatch(AGESA_STRUCT_NAME func, AMD_CONFIG_PARAMS *StdHeader);

void platform_BeforeInitReset(struct sysinfo *cb, AMD_RESET_PARAMS *Reset);
void board_BeforeInitReset(struct sysinfo *cb, AMD_RESET_PARAMS *Reset);

void platform_BeforeInitEarly(struct sysinfo *cb, AMD_EARLY_PARAMS *Early);
void board_BeforeInitEarly(struct sysinfo *cb, AMD_EARLY_PARAMS *Early);

/* Normal boot */
void platform_BeforeInitPost(struct sysinfo *cb, AMD_POST_PARAMS *Post);
void board_BeforeInitPost(struct sysinfo *cb, AMD_POST_PARAMS *Post);
void platform_AfterInitPost(struct sysinfo *cb, AMD_POST_PARAMS *Post);

void platform_BeforeInitEnv(struct sysinfo *cb, AMD_ENV_PARAMS *Env);
void board_BeforeInitEnv(struct sysinfo *cb, AMD_ENV_PARAMS *Env);
void platform_AfterInitEnv(struct sysinfo *cb, AMD_ENV_PARAMS *Env);

void platform_BeforeInitMid(struct sysinfo *cb, AMD_MID_PARAMS *Mid);
void board_BeforeInitMid(struct sysinfo *cb, AMD_MID_PARAMS *Mid);

void platform_AfterInitLate(struct sysinfo *cb, AMD_LATE_PARAMS *Late);
void completion_InitLate(struct sysinfo *cb, AMD_LATE_PARAMS *Late);

/* S3 Resume */
void platform_BeforeInitResume(struct sysinfo *cb, AMD_RESUME_PARAMS *Resume);
void platform_AfterInitResume(struct sysinfo *cb, AMD_RESUME_PARAMS *Resume);

void platform_BeforeS3LateRestore(struct sysinfo *cb, AMD_S3LATE_PARAMS *S3Late);
void platform_AfterS3LateRestore(struct sysinfo *cb, AMD_S3LATE_PARAMS *S3Late);

#if IS_ENABLED(CONFIG_CPU_AMD_PI_00660F01)
typedef void AMD_S3SAVE_PARAMS;
#endif
void platform_AfterS3Save(struct sysinfo *cb, AMD_S3SAVE_PARAMS *S3Save);

/* FCH callouts, not used with CIMx. */
#define HAS_AGESA_FCH_OEM_CALLOUT \
	IS_ENABLED(CONFIG_SOUTHBRIDGE_AMD_AGESA_HUDSON) || \
	IS_ENABLED(CONFIG_SOUTHBRIDGE_AMD_AGESA_YANGTZE) || \
	IS_ENABLED(CONFIG_SOUTHBRIDGE_AMD_AGESA_BOLTON) || \
	IS_ENABLED(CONFIG_SOUTHBRIDGE_AMD_PI_AVALON) || \
	IS_ENABLED(CONFIG_SOUTHBRIDGE_AMD_PI_BOLTON) || \
	IS_ENABLED(CONFIG_SOUTHBRIDGE_AMD_PI_KERN)

#if HAS_AGESA_FCH_OEM_CALLOUT
/* FIXME:  Structures included here were supposed to be private to AGESA. */
#include <FchCommonCfg.h>
void agesa_fch_oem_config(uintptr_t Data, AMD_CONFIG_PARAMS *StdHeader);
void board_FCH_InitReset(struct sysinfo *cb, FCH_RESET_DATA_BLOCK *FchReset);
void board_FCH_InitEnv(struct sysinfo *cb, FCH_DATA_BLOCK *FchEnv);
#endif

#endif /* _STATE_MACHINE_H_ */
