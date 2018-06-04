/*
 * This file is part of the coreboot project.
 *
 * Copyright (C) 2011 The ChromiumOS Authors.  All rights reserved.
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

#ifndef __TIMESTAMP_SERIALIZED_H__
#define __TIMESTAMP_SERIALIZED_H__

#include <stdint.h>
#include <compiler.h>

struct timestamp_entry {
	uint32_t	entry_id;
	uint64_t	entry_stamp;
} __packed;

struct timestamp_table {
	uint64_t	base_time;
	uint16_t	max_entries;
	uint16_t	tick_freq_mhz;
	uint32_t	num_entries;
	struct timestamp_entry entries[0]; /* Variable number of entries */
} __packed;

enum timestamp_id {
	TS_START_ROMSTAGE = 1,
	TS_BEFORE_INITRAM = 2,
	TS_AFTER_INITRAM = 3,
	TS_END_ROMSTAGE = 4,
	TS_START_VBOOT = 5,
	TS_END_VBOOT = 6,
	TS_START_COPYRAM = 8,
	TS_END_COPYRAM = 9,
	TS_START_RAMSTAGE = 10,
	TS_START_BOOTBLOCK = 11,
	TS_END_BOOTBLOCK = 12,
	TS_START_COPYROM = 13,
	TS_END_COPYROM = 14,
	TS_START_ULZMA = 15,
	TS_END_ULZMA = 16,
	TS_START_ULZ4F = 17,
	TS_END_ULZ4F = 18,
	TS_DEVICE_ENUMERATE = 30,
	TS_DEVICE_CONFIGURE = 40,
	TS_DEVICE_ENABLE = 50,
	TS_DEVICE_INITIALIZE = 60,
	TS_OPROM_INITIALIZE = 65,
	TS_OPROM_COPY_END = 66,
	TS_OPROM_END = 67,
	TS_DEVICE_DONE = 70,
	TS_CBMEM_POST = 75,
	TS_WRITE_TABLES = 80,
	TS_FINALIZE_CHIPS = 85,
	TS_LOAD_PAYLOAD = 90,
	TS_ACPI_WAKE_JUMP = 98,
	TS_SELFBOOT_JUMP = 99,

	/* 500+ reserved for vendorcode extensions (500-600: google/chromeos) */
	TS_START_COPYVER = 501,
	TS_END_COPYVER = 502,
	TS_START_TPMINIT = 503,
	TS_END_TPMINIT = 504,
	TS_START_VERIFY_SLOT = 505,
	TS_END_VERIFY_SLOT = 506,
	TS_START_HASH_BODY = 507,
	TS_DONE_LOADING = 508,
	TS_DONE_HASHING = 509,
	TS_END_HASH_BODY = 510,
	TS_START_TPMPCR = 511,
	TS_END_TPMPCR = 512,
	TS_START_TPMLOCK = 513,
	TS_END_TPMLOCK = 514,
	TS_START_COPYVPD = 550,
	TS_END_COPYVPD_RO = 551,
	TS_END_COPYVPD_RW = 552,

	/* 900-920 reserved for vendorcode extensions (900-940: AMD AGESA) */
	TS_AGESA_INIT_RESET_START = 900,
	TS_AGESA_INIT_RESET_DONE = 901,
	TS_AGESA_INIT_EARLY_START = 902,
	TS_AGESA_INIT_EARLY_DONE = 903,
	TS_AGESA_INIT_POST_START = 904,
	TS_AGESA_INIT_POST_DONE = 905,
	TS_AGESA_INIT_ENV_START = 906,
	TS_AGESA_INIT_ENV_DONE = 907,
	TS_AGESA_INIT_MID_START = 908,
	TS_AGESA_INIT_MID_DONE = 909,
	TS_AGESA_INIT_LATE_START = 910,
	TS_AGESA_INIT_LATE_DONE = 911,
	TS_AGESA_INIT_RTB_START = 912,
	TS_AGESA_INIT_RTB_DONE = 913,
	TS_AGESA_INIT_RESUME_START = 914,
	TS_AGESA_INIT_RESUME_DONE = 915,
	TS_AGESA_S3_LATE_START = 916,
	TS_AGESA_S3_LATE_DONE = 917,
	TS_AGESA_S3_FINAL_START = 918,
	TS_AGESA_S3_FINAL_DONE = 919,

	/* 940-950 reserved for vendorcode extensions (940-950: Intel ME) */
	TS_ME_INFORM_DRAM_WAIT = 940,
	TS_ME_INFORM_DRAM_DONE = 941,

	/* 950+ reserved for vendorcode extensions (950-999: intel/fsp) */
	TS_FSP_MEMORY_INIT_START = 950,
	TS_FSP_MEMORY_INIT_END = 951,
	TS_FSP_TEMP_RAM_EXIT_START = 952,
	TS_FSP_TEMP_RAM_EXIT_END = 953,
	TS_FSP_SILICON_INIT_START = 954,
	TS_FSP_SILICON_INIT_END = 955,
	TS_FSP_BEFORE_ENUMERATE = 956,
	TS_FSP_AFTER_ENUMERATE = 957,
	TS_FSP_BEFORE_FINALIZE = 958,
	TS_FSP_AFTER_FINALIZE = 959,
	TS_FSP_BEFORE_END_OF_FIRMWARE = 960,
	TS_FSP_AFTER_END_OF_FIRMWARE = 961,

	/* 1000+ reserved for payloads (1000-1200: ChromeOS depthcharge) */

	/* Depthcharge entry IDs start at 1000 */
	TS_DC_START = 1000,

	TS_RO_PARAMS_INIT = 1001,
	TS_RO_VB_INIT = 1002,
	TS_RO_VB_SELECT_FIRMWARE = 1003,
	TS_RO_VB_SELECT_AND_LOAD_KERNEL = 1004,

	TS_RW_VB_SELECT_AND_LOAD_KERNEL = 1010,

	TS_VB_SELECT_AND_LOAD_KERNEL = 1020,
	TS_VB_EC_VBOOT_DONE = 1030,
	TS_VB_STORAGE_INIT_DONE = 1040,
	TS_VB_READ_KERNEL_DONE = 1050,
	TS_VB_VBOOT_DONE = 1100,

	TS_START_KERNEL = 1101,
	TS_KERNEL_DECOMPRESSION = 1102,
};

static const struct timestamp_id_to_name {
	uint32_t id;
	const char *name;
} timestamp_ids[] = {
	/* Marker to report base_time. */
	{ 0,			"1st timestamp" },
	{ TS_START_ROMSTAGE,	"start of romstage" },
	{ TS_BEFORE_INITRAM,	"before ram initialization" },
	{ TS_AFTER_INITRAM,	"after ram initialization" },
	{ TS_END_ROMSTAGE,	"end of romstage" },
	{ TS_START_VBOOT,	"start of verified boot" },
	{ TS_END_VBOOT,		"end of verified boot" },
	{ TS_START_COPYRAM,	"starting to load ramstage" },
	{ TS_END_COPYRAM,	"finished loading ramstage" },
	{ TS_START_RAMSTAGE,	"start of ramstage" },
	{ TS_START_BOOTBLOCK,	"start of bootblock" },
	{ TS_END_BOOTBLOCK,	"end of bootblock" },
	{ TS_START_COPYROM,	"starting to load romstage" },
	{ TS_END_COPYROM,	"finished loading romstage" },
	{ TS_START_ULZMA,	"starting LZMA decompress (ignore for x86)" },
	{ TS_END_ULZMA,		"finished LZMA decompress (ignore for x86)" },
	{ TS_START_ULZ4F,	"starting LZ4 decompress (ignore for x86)" },
	{ TS_END_ULZ4F,		"finished LZ4 decompress (ignore for x86)" },
	{ TS_DEVICE_ENUMERATE,	"device enumeration" },
	{ TS_DEVICE_CONFIGURE,	"device configuration" },
	{ TS_DEVICE_ENABLE,	"device enable" },
	{ TS_DEVICE_INITIALIZE,	"device initialization" },
	{ TS_OPROM_INITIALIZE,	"Option ROM initialization" },
	{ TS_OPROM_COPY_END,	"Option ROM copy done" },
	{ TS_OPROM_END,		"Option ROM run done"   },
	{ TS_DEVICE_DONE,	"device setup done" },
	{ TS_CBMEM_POST,	"cbmem post" },
	{ TS_WRITE_TABLES,	"write tables" },
	{ TS_FINALIZE_CHIPS,	"finalize chips" },
	{ TS_LOAD_PAYLOAD,	"load payload" },
	{ TS_ACPI_WAKE_JUMP,	"ACPI wake jump" },
	{ TS_SELFBOOT_JUMP,	"selfboot jump" },

	{ TS_START_COPYVER,	"starting to load verstage" },
	{ TS_END_COPYVER,	"finished loading verstage" },
	{ TS_START_TPMINIT,	"starting to initialize TPM" },
	{ TS_END_TPMINIT,	"finished TPM initialization" },
	{ TS_START_VERIFY_SLOT,	"starting to verify keyblock/preamble (RSA)" },
	{ TS_END_VERIFY_SLOT,	"finished verifying keyblock/preamble (RSA)" },
	{ TS_START_HASH_BODY,	"starting to verify body (load+SHA2+RSA) " },
	{ TS_DONE_LOADING,	"finished loading body (ignore for x86)" },
	{ TS_DONE_HASHING,	"finished calculating body hash (SHA2)" },
	{ TS_END_HASH_BODY,	"finished verifying body signature (RSA)" },
	{ TS_START_TPMPCR,	"starting TPM PCR extend" },
	{ TS_END_TPMPCR,	"finished TPM PCR extend" },
	{ TS_START_TPMLOCK,	"starting locking TPM" },
	{ TS_END_TPMLOCK,	"finished locking TPM" },

	{ TS_START_COPYVPD,	"starting to load Chrome OS VPD" },
	{ TS_END_COPYVPD_RO,	"finished loading Chrome OS VPD (RO)" },
	{ TS_END_COPYVPD_RW,	"finished loading Chrome OS VPD (RW)" },

	{ TS_DC_START,		"depthcharge start" },
	{ TS_RO_PARAMS_INIT,	"RO parameter init" },
	{ TS_RO_VB_INIT,	"RO vboot init" },
	{ TS_RO_VB_SELECT_FIRMWARE,		"RO vboot select firmware" },
	{ TS_RO_VB_SELECT_AND_LOAD_KERNEL,	"RO vboot select&load kernel" },
	{ TS_RW_VB_SELECT_AND_LOAD_KERNEL,	"RW vboot select&load kernel" },
	{ TS_VB_SELECT_AND_LOAD_KERNEL,		"vboot select&load kernel" },
	{ TS_VB_EC_VBOOT_DONE,	"finished EC verification" },
	{ TS_VB_STORAGE_INIT_DONE, "finished storage device initialization" },
	{ TS_VB_READ_KERNEL_DONE, "finished reading kernel from disk" },
	{ TS_VB_VBOOT_DONE,	"finished vboot kernel verification" },
	{ TS_KERNEL_DECOMPRESSION, "starting kernel decompression/relocation" },
	{ TS_START_KERNEL,	"jumping to kernel" },

	/* AMD AGESA related timestamps */
	{ TS_AGESA_INIT_RESET_START,	"calling AmdInitReset" },
	{ TS_AGESA_INIT_RESET_DONE,	"back from AmdInitReset" },
	{ TS_AGESA_INIT_EARLY_START,	"calling AmdInitEarly" },
	{ TS_AGESA_INIT_EARLY_DONE,	"back from AmdInitEarly" },
	{ TS_AGESA_INIT_POST_START,	"calling AmdInitPost" },
	{ TS_AGESA_INIT_POST_DONE,	"back from AmdInitPost" },
	{ TS_AGESA_INIT_ENV_START,	"calling AmdInitEnv" },
	{ TS_AGESA_INIT_ENV_DONE,	"back from AmdInitEnv" },
	{ TS_AGESA_INIT_MID_START,	"calling AmdInitMid" },
	{ TS_AGESA_INIT_MID_DONE,	"back from AmdInitMid" },
	{ TS_AGESA_INIT_LATE_START,	"calling AmdInitLate" },
	{ TS_AGESA_INIT_LATE_DONE,	"back from AmdInitLate" },
	{ TS_AGESA_INIT_RTB_START,	"calling AmdInitRtb/AmdS3Save" },
	{ TS_AGESA_INIT_RTB_DONE,	"back from AmdInitRtb/AmdS3Save" },
	{ TS_AGESA_INIT_RESUME_START,	"calling AmdInitResume" },
	{ TS_AGESA_INIT_RESUME_DONE,	"back from AmdInitResume" },
	{ TS_AGESA_S3_LATE_START,	"calling AmdS3LateRestore" },
	{ TS_AGESA_S3_LATE_DONE,	"back from AmdS3LateRestore" },
	{ TS_AGESA_S3_FINAL_START,	"calling AmdS3FinalRestore" },
	{ TS_AGESA_S3_FINAL_DONE,	"back from AmdS3FinalRestore" },

	/* Intel ME related timestamps */
	{ TS_ME_INFORM_DRAM_WAIT,	"waiting for ME acknowledgement of raminit"},
	{ TS_ME_INFORM_DRAM_DONE,	"finished waiting for ME response"},

	/* FSP related timestamps */
	{ TS_FSP_MEMORY_INIT_START, "calling FspMemoryInit" },
	{ TS_FSP_MEMORY_INIT_END, "returning from FspMemoryInit" },
	{ TS_FSP_TEMP_RAM_EXIT_START, "calling FspTempRamExit" },
	{ TS_FSP_TEMP_RAM_EXIT_END, "returning from FspTempRamExit" },
	{ TS_FSP_SILICON_INIT_START, "calling FspSiliconInit" },
	{ TS_FSP_SILICON_INIT_END, "returning from FspSiliconInit" },
	{ TS_FSP_BEFORE_ENUMERATE, "calling FspNotify(AfterPciEnumeration)" },
	{ TS_FSP_AFTER_ENUMERATE,
		 "returning from FspNotify(AfterPciEnumeration)" },
	{ TS_FSP_BEFORE_FINALIZE, "calling FspNotify(ReadyToBoot)" },
	{ TS_FSP_AFTER_FINALIZE, "returning from FspNotify(ReadyToBoot)" },
	{ TS_FSP_BEFORE_END_OF_FIRMWARE, "calling FspNotify(EndOfFirmware)" },
	{ TS_FSP_AFTER_END_OF_FIRMWARE,
		"returning from FspNotify(EndOfFirmware)" },
};

#endif
