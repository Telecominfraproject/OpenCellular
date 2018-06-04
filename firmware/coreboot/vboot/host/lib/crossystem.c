/* Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>

#include "2sysincludes.h"
#include "2api.h"
#include "2nvstorage.h"

#include "host_common.h"

#include "crossystem.h"
#include "crossystem_arch.h"
#include "crossystem_vbnv.h"
#include "utility.h"
#include "vboot_common.h"
#include "vboot_struct.h"

/* Filename for kernel command line */
#define KERNEL_CMDLINE_PATH "/proc/cmdline"

#define MOSYS_CROS_PATH "/usr/sbin/mosys"
#define MOSYS_ANDROID_PATH "/system/bin/mosys"

/* Fields that GetVdatString() can get */
typedef enum VdatStringField {
	VDAT_STRING_TIMERS = 0,           /* Timer values */
	VDAT_STRING_LOAD_FIRMWARE_DEBUG,  /* LoadFirmware() debug information */
	VDAT_STRING_LOAD_KERNEL_DEBUG,    /* LoadKernel() debug information */
	VDAT_STRING_MAINFW_ACT            /* Active main firmware */
} VdatStringField;


/* Fields that GetVdatInt() can get */
typedef enum VdatIntField {
	VDAT_INT_FLAGS = 0,           /* Flags */
	VDAT_INT_HEADER_VERSION,      /* Header version for VbSharedData */
	VDAT_INT_DEVSW_BOOT,          /* Dev switch position at boot */
	VDAT_INT_DEVSW_VIRTUAL,       /* Dev switch is virtual */
	VDAT_INT_RECSW_BOOT,          /* Recovery switch position at boot */
	VDAT_INT_HW_WPSW_BOOT,        /* Hardware WP switch position at boot */

	VDAT_INT_FW_VERSION_TPM,      /* Current firmware version in TPM */
	VDAT_INT_KERNEL_VERSION_TPM,  /* Current kernel version in TPM */
	VDAT_INT_TRIED_FIRMWARE_B,    /* Tried firmware B due to fwb_tries */
	VDAT_INT_KERNEL_KEY_VERIFIED, /* Kernel key verified using
				       * signature, not just hash */
	VDAT_INT_RECOVERY_REASON,     /* Recovery reason for current boot */
	VDAT_INT_FW_BOOT2             /* Firmware selection by vboot2 */
} VdatIntField;


/* Description of build options that may be specified on the
 * kernel command line. */
typedef enum VbBuildOption {
	VB_BUILD_OPTION_UNKNOWN,
	VB_BUILD_OPTION_DEBUG,
	VB_BUILD_OPTION_NODEBUG
} VbBuildOption;

static const char *fw_results[] = {"unknown", "trying", "success", "failure"};
static const char *default_boot[] = {"disk", "usb", "legacy"};

/* Masks for kern_nv usage by kernel. */
#define KERN_NV_FWUPDATE_TRIES_MASK 0x000F
#define KERN_NV_BLOCK_DEVMODE_FLAG  0x0010
#define KERN_NV_TPM_ATTACK_FLAG     0x0020
/* If you want to use the remaining currently-unused bits in kern_nv
 * for something kernel-y, define a new field (the way we did for
 * fwupdate_tries).  Don't just modify kern_nv directly, because that
 * makes it too easy to accidentally corrupt other sub-fields. */
#define KERN_NV_CURRENTLY_UNUSED    0xFFC0

/* Return true if the FWID starts with the specified string. */
int FwidStartsWith(const char *start)
{
	char fwid[VB_MAX_STRING_PROPERTY];
	if (!VbGetSystemPropertyString("fwid", fwid, sizeof(fwid)))
		return 0;

	return 0 == strncmp(fwid, start, strlen(start));
}

static int vnc_read;

int vb2_get_nv_storage(enum vb2_nv_param param)
{
	VbSharedDataHeader* sh = VbSharedDataRead();
	static struct vb2_context cached_ctx;

	/* TODO: locking around NV access */
	if (!vnc_read) {
		memset(&cached_ctx, 0, sizeof(cached_ctx));
		if (sh && sh->flags & VBSD_NVDATA_V2)
			cached_ctx.flags |= VB2_CONTEXT_NVDATA_V2;
		if (0 != vb2_read_nv_storage(&cached_ctx))
			return -1;
		vb2_nv_init(&cached_ctx);

		/* TODO: If vnc.raw_changed, attempt to reopen NVRAM for write
		 * and save the new defaults.  If we're able to, log. */

		vnc_read = 1;
	}

	return (int)vb2_nv_get(&cached_ctx, param);
}

int vb2_set_nv_storage(enum vb2_nv_param param, int value)
{
	VbSharedDataHeader* sh = VbSharedDataRead();
	struct vb2_context ctx;

	/* TODO: locking around NV access */
	memset(&ctx, 0, sizeof(ctx));
	if (sh && sh->flags & VBSD_NVDATA_V2)
		ctx.flags |= VB2_CONTEXT_NVDATA_V2;
	if (0 != vb2_read_nv_storage(&ctx))
		return -1;
	vb2_nv_init(&ctx);
	vb2_nv_set(&ctx, param, (uint32_t)value);

	if (ctx.flags & VB2_CONTEXT_NVDATA_CHANGED) {
		vnc_read = 0;
		if (0 != vb2_write_nv_storage(&ctx))
			return -1;
	}

	/* Success */
	return 0;
}

/*
 * Set a param value, and try to flag it for persistent backup.  It's okay if
 * backup isn't supported (which it isn't, in current designs). It's
 * best-effort only.
 */
static int vb2_set_nv_storage_with_backup(enum vb2_nv_param param, int value)
{
	int retval;
	retval = vb2_set_nv_storage(param, value);
	if (!retval)
		vb2_set_nv_storage(VB2_NV_BACKUP_NVRAM_REQUEST, 1);
	return retval;
}

/* Find what build/debug status is specified on the kernel command
 * line, if any. */
static VbBuildOption VbScanBuildOption(void)
{
	FILE* f = NULL;
	char buf[4096] = "";
	char *t, *saveptr;
	const char *delimiters = " \r\n";

	f = fopen(KERNEL_CMDLINE_PATH, "r");
	if (NULL != f) {
		if (NULL == fgets(buf, sizeof(buf), f))
			buf[0] = 0;
		fclose(f);
	}
	for (t = strtok_r(buf, delimiters, &saveptr); t;
	     t = strtok_r(NULL, delimiters, &saveptr)) {
		if (0 == strcmp(t, "cros_debug"))
			return VB_BUILD_OPTION_DEBUG;
		else if (0 == strcmp(t, "cros_nodebug"))
			return VB_BUILD_OPTION_NODEBUG;
	}

	return VB_BUILD_OPTION_UNKNOWN;
}

/* Determine whether the running OS image was built for debugging.
 * Returns 1 if yes, 0 if no or indeterminate. */
int VbGetDebugBuild(void)
{
	return VB_BUILD_OPTION_DEBUG == VbScanBuildOption();
}

/* Determine whether OS-level debugging should be allowed.
 * Returns 1 if yes, 0 if no or indeterminate. */
int VbGetCrosDebug(void)
{
	/* If the currently running system specifies its debug status, use
	 * that in preference to other indicators. */
	VbBuildOption option = VbScanBuildOption();
	if (VB_BUILD_OPTION_DEBUG == option) {
		return 1;
	} else if (VB_BUILD_OPTION_NODEBUG == option) {
		return 0;
	}

	/* Command line is silent; allow debug if the dev switch is on. */
	if (1 == VbGetSystemPropertyInt("devsw_boot"))
		return 1;

	/* All other cases disallow debug. */
	return 0;
}

char *GetVdatLoadFirmwareDebug(char *dest, int size,
                               const VbSharedDataHeader *sh)
{
	snprintf(dest, size,
		 "Check A result=%d\n"
		 "Check B result=%d\n"
		 "Firmware index booted=0x%02x\n"
		 "TPM combined version at start=0x%08x\n"
		 "Lowest combined version from firmware=0x%08x\n",
		 sh->check_fw_a_result,
		 sh->check_fw_b_result,
		 sh->firmware_index,
		 sh->fw_version_tpm_start,
		 sh->fw_version_lowest);
	return dest;
}

#define TRUNCATED "\n(truncated)\n"

char *GetVdatLoadKernelDebug(char *dest, int size,
                             const VbSharedDataHeader *sh)
{
	int used = 0;
	int first_call_tracked = 0;
	int call;

	/* Make sure we have space for truncation warning */
	if (size < strlen(TRUNCATED) + 1)
		return NULL;
	size -= strlen(TRUNCATED) + 1;

	used += snprintf(
	    dest + used, size - used,
	    "Calls to LoadKernel()=%d\n",
	    sh->lk_call_count);
	if (used > size)
		goto LoadKernelDebugExit;

	/* Report on the last calls */
	if (sh->lk_call_count > VBSD_MAX_KERNEL_CALLS)
		first_call_tracked = sh->lk_call_count - VBSD_MAX_KERNEL_CALLS;
	for (call = first_call_tracked; call < sh->lk_call_count; call++) {
		const VbSharedDataKernelCall* shc = sh->lk_calls +
				(call & (VBSD_MAX_KERNEL_CALLS - 1));
		int first_part_tracked = 0;
		int part;

		used += snprintf(dest + used, size - used,
				 "Call %d:\n"
				 "  Boot flags=0x%02x\n"
				 "  Boot mode=%d\n"
				 "  Test error=%d\n"
				 "  Return code=%d\n"
				 "  Debug flags=0x%02x\n"
				 "  Drive sectors=%" PRIu64 "\n"
				 "  Sector size=%d\n"
				 "  Check result=%d\n"
				 "  Kernel partitions found=%d\n",
				 call + 1,
				 shc->boot_flags,
				 shc->boot_mode,
				 shc->test_error_num,
				 shc->return_code,
				 shc->flags,
				 shc->sector_count,
				 shc->sector_size,
				 shc->check_result,
				 shc->kernel_parts_found);
		if (used > size)
			goto LoadKernelDebugExit;

		/* If we found too many partitions, only prints ones where the
		 * structure has info. */
		if (shc->kernel_parts_found > VBSD_MAX_KERNEL_PARTS)
			first_part_tracked = shc->kernel_parts_found -
					VBSD_MAX_KERNEL_PARTS;

		/* Report on the partitions checked */
		for (part = first_part_tracked; part < shc->kernel_parts_found;
		     part++) {
			const VbSharedDataKernelPart* shp = shc->parts +
					(part & (VBSD_MAX_KERNEL_PARTS - 1));

			used += snprintf(dest + used, size - used,
					 "  Kernel %d:\n"
					 "    GPT index=%d\n"
					 "    Start sector=%" PRIu64 "\n"
					 "    Sector count=%" PRIu64 "\n"
					 "    Combined version=0x%08x\n"
					 "    Check result=%d\n"
					 "    Debug flags=0x%02x\n",
					 part + 1,
					 shp->gpt_index,
					 shp->sector_start,
					 shp->sector_count,
					 shp->combined_version,
					 shp->check_result,
					 shp->flags);
			if (used > size)
				goto LoadKernelDebugExit;
		}
	}

LoadKernelDebugExit:

	/* Warn if data was truncated; we left space for this above. */
	if (used > size)
		strcat(dest, TRUNCATED);

	return dest;
}

char *GetVdatString(char *dest, int size, VdatStringField field)
{
	VbSharedDataHeader *sh = VbSharedDataRead();
	char *value = dest;

	if (!sh)
		return NULL;

	switch (field) {
		case VDAT_STRING_TIMERS:
			snprintf(dest, size,
				 "LFS=%" PRIu64 ",%" PRIu64
				 " LF=%" PRIu64 ",%" PRIu64
				 " LK=%" PRIu64 ",%" PRIu64,
				 sh->timer_vb_init_enter,
				 sh->timer_vb_init_exit,
				 sh->timer_vb_select_firmware_enter,
				 sh->timer_vb_select_firmware_exit,
				 sh->timer_vb_select_and_load_kernel_enter,
				 sh->timer_vb_select_and_load_kernel_exit);
			break;

		case VDAT_STRING_LOAD_FIRMWARE_DEBUG:
			value = GetVdatLoadFirmwareDebug(dest, size, sh);
			break;

		case VDAT_STRING_LOAD_KERNEL_DEBUG:
			value = GetVdatLoadKernelDebug(dest, size, sh);
			break;

		case VDAT_STRING_MAINFW_ACT:
			switch(sh->firmware_index) {
				case 0:
					StrCopy(dest, "A", size);
					break;
				case 1:
					StrCopy(dest, "B", size);
					break;
				case 0xFF:
					StrCopy(dest, "recovery", size);
					break;
				default:
					value = NULL;
			}
			break;

		default:
			value = NULL;
			break;
	}

	free(sh);
	return value;
}

int GetVdatInt(VdatIntField field)
{
	VbSharedDataHeader* sh = VbSharedDataRead();
	int value = -1;

	if (!sh)
		return -1;

	/* Fields supported in version 1 */
	switch (field) {
		case VDAT_INT_FLAGS:
			value = (int)sh->flags;
			break;
		case VDAT_INT_HEADER_VERSION:
			value = sh->struct_version;
			break;
		case VDAT_INT_TRIED_FIRMWARE_B:
			value = (sh->flags & VBSD_FWB_TRIED ? 1 : 0);
			break;
		case VDAT_INT_KERNEL_KEY_VERIFIED:
			value = (sh->flags & VBSD_KERNEL_KEY_VERIFIED ? 1 : 0);
			break;
		case VDAT_INT_FW_VERSION_TPM:
			value = (int)sh->fw_version_tpm;
			break;
		case VDAT_INT_KERNEL_VERSION_TPM:
			value = (int)sh->kernel_version_tpm;
			break;
		case VDAT_INT_FW_BOOT2:
			value = (sh->flags & VBSD_BOOT_FIRMWARE_VBOOT2 ? 1 : 0);
		default:
			break;
	}

	/* Fields added in struct version 2 */
	if (sh->struct_version >= 2) {
		switch(field) {
			case VDAT_INT_DEVSW_BOOT:
				value = (sh->flags &
					 VBSD_BOOT_DEV_SWITCH_ON ? 1 : 0);
				break;
			case VDAT_INT_DEVSW_VIRTUAL:
				value = (sh->flags &
					 VBSD_HONOR_VIRT_DEV_SWITCH ? 1 : 0);
				break;
			case VDAT_INT_RECSW_BOOT:
				value = (sh->flags &
					 VBSD_BOOT_REC_SWITCH_ON ? 1 : 0);
				break;
			case VDAT_INT_HW_WPSW_BOOT:
				value = (sh->flags &
					 VBSD_BOOT_FIRMWARE_WP_ENABLED ? 1 : 0);
				break;
			case VDAT_INT_RECOVERY_REASON:
				value = sh->recovery_reason;
				break;
			default:
				break;
		}
	}

	free(sh);
	return value;
}

/* Return version of VbSharedData struct or -1 if not found. */
int VbSharedDataVersion(void)
{
	return GetVdatInt(VDAT_INT_HEADER_VERSION);
}

int VbGetSystemPropertyInt(const char *name)
{
	int value = -1;

	/* Check architecture-dependent properties first */
	value = VbGetArchPropertyInt(name);
	if (-1 != value)
		return value;

	/* NV storage values */
	else if (!strcasecmp(name,"kern_nv")) {
		value = vb2_get_nv_storage(VB2_NV_KERNEL_FIELD);
	} else if (!strcasecmp(name,"nvram_cleared")) {
		value = vb2_get_nv_storage(VB2_NV_KERNEL_SETTINGS_RESET);
	} else if (!strcasecmp(name,"recovery_request")) {
		value = vb2_get_nv_storage(VB2_NV_RECOVERY_REQUEST);
	} else if (!strcasecmp(name,"dbg_reset")) {
		value = vb2_get_nv_storage(VB2_NV_DEBUG_RESET_MODE);
	} else if (!strcasecmp(name,"disable_dev_request")) {
		value = vb2_get_nv_storage(VB2_NV_DISABLE_DEV_REQUEST);
	} else if (!strcasecmp(name,"clear_tpm_owner_request")) {
		value = vb2_get_nv_storage(VB2_NV_CLEAR_TPM_OWNER_REQUEST);
	} else if (!strcasecmp(name,"clear_tpm_owner_done")) {
		value = vb2_get_nv_storage(VB2_NV_CLEAR_TPM_OWNER_DONE);
	} else if (!strcasecmp(name,"tpm_rebooted")) {
		value = vb2_get_nv_storage(VB2_NV_TPM_REQUESTED_REBOOT);
	} else if (!strcasecmp(name,"fwb_tries") ||
		   !strcasecmp(name,"fw_try_count")) {
		value = vb2_get_nv_storage(VB2_NV_TRY_COUNT);
	} else if (!strcasecmp(name,"fw_vboot2")) {
		value = GetVdatInt(VDAT_INT_FW_BOOT2);
	} else if (!strcasecmp(name,"fwupdate_tries")) {
		value = vb2_get_nv_storage(VB2_NV_KERNEL_FIELD);
		if (value != -1)
			value &= KERN_NV_FWUPDATE_TRIES_MASK;
	} else if (!strcasecmp(name,"block_devmode")) {
		value = vb2_get_nv_storage(VB2_NV_KERNEL_FIELD);
		if (value != -1) {
			value &= KERN_NV_BLOCK_DEVMODE_FLAG;
			value = !!value;
		}
	} else if (!strcasecmp(name,"tpm_attack")) {
		value = vb2_get_nv_storage(VB2_NV_KERNEL_FIELD);
		if (value != -1) {
			value &= KERN_NV_TPM_ATTACK_FLAG;
			value = !!value;
		}
	} else if (!strcasecmp(name,"loc_idx")) {
		value = vb2_get_nv_storage(VB2_NV_LOCALIZATION_INDEX);
	} else if (!strcasecmp(name,"backup_nvram_request")) {
		value = vb2_get_nv_storage(VB2_NV_BACKUP_NVRAM_REQUEST);
	} else if (!strcasecmp(name,"dev_boot_usb")) {
		value = vb2_get_nv_storage(VB2_NV_DEV_BOOT_USB);
	} else if (!strcasecmp(name,"dev_boot_legacy")) {
		value = vb2_get_nv_storage(VB2_NV_DEV_BOOT_LEGACY);
	} else if (!strcasecmp(name,"dev_boot_signed_only")) {
		value = vb2_get_nv_storage(VB2_NV_DEV_BOOT_SIGNED_ONLY);
	} else if (!strcasecmp(name,"dev_boot_fastboot_full_cap")) {
		value = vb2_get_nv_storage(VB2_NV_DEV_BOOT_FASTBOOT_FULL_CAP);
	} else if (!strcasecmp(name,"dev_enable_udc")) {
		value = vb2_get_nv_storage(VB2_NV_DEV_ENABLE_UDC);
	} else if (!strcasecmp(name,"oprom_needed")) {
		value = vb2_get_nv_storage(VB2_NV_OPROM_NEEDED);
	} else if (!strcasecmp(name,"recovery_subcode")) {
		value = vb2_get_nv_storage(VB2_NV_RECOVERY_SUBCODE);
	} else if (!strcasecmp(name,"wipeout_request")) {
		value = vb2_get_nv_storage(VB2_NV_REQ_WIPEOUT);
	} else if (!strcasecmp(name,"kernel_max_rollforward")) {
		value = vb2_get_nv_storage(VB2_NV_KERNEL_MAX_ROLLFORWARD);
	}
	/* Other parameters */
	else if (!strcasecmp(name,"cros_debug")) {
		value = VbGetCrosDebug();
	} else if (!strcasecmp(name,"debug_build")) {
		value = VbGetDebugBuild();
	} else if (!strcasecmp(name,"devsw_boot")) {
		value = GetVdatInt(VDAT_INT_DEVSW_BOOT);
	} else if (!strcasecmp(name,"devsw_virtual")) {
		value = GetVdatInt(VDAT_INT_DEVSW_VIRTUAL);
	} else if (!strcasecmp(name, "recoverysw_boot")) {
		value = GetVdatInt(VDAT_INT_RECSW_BOOT);
	} else if (!strcasecmp(name, "wpsw_boot")) {
		value = GetVdatInt(VDAT_INT_HW_WPSW_BOOT);
	} else if (!strcasecmp(name,"vdat_flags")) {
		value = GetVdatInt(VDAT_INT_FLAGS);
	} else if (!strcasecmp(name,"tpm_fwver")) {
		value = GetVdatInt(VDAT_INT_FW_VERSION_TPM);
	} else if (!strcasecmp(name,"tpm_kernver")) {
		value = GetVdatInt(VDAT_INT_KERNEL_VERSION_TPM);
	} else if (!strcasecmp(name,"tried_fwb")) {
		value = GetVdatInt(VDAT_INT_TRIED_FIRMWARE_B);
	} else if (!strcasecmp(name,"recovery_reason")) {
		value = GetVdatInt(VDAT_INT_RECOVERY_REASON);
	} else if (!strcasecmp(name, "fastboot_unlock_in_fw")) {
		value = vb2_get_nv_storage(VB2_NV_FASTBOOT_UNLOCK_IN_FW);
	} else if (!strcasecmp(name, "boot_on_ac_detect")) {
		value = vb2_get_nv_storage(VB2_NV_BOOT_ON_AC_DETECT);
	} else if (!strcasecmp(name, "try_ro_sync")) {
		value = vb2_get_nv_storage(VB2_NV_TRY_RO_SYNC);
	} else if (!strcasecmp(name, "battery_cutoff_request")) {
		value = vb2_get_nv_storage(VB2_NV_BATTERY_CUTOFF_REQUEST);
	} else if (!strcasecmp(name, "inside_vm")) {
		/* Detect if the host is a VM. If there is no HWID and the
		 * firmware type is "nonchrome", then assume it is a VM. If
		 * HWID is present, it is a baremetal Chrome OS machine. Other
		 * cases are errors. */
		char hwid[VB_MAX_STRING_PROPERTY];
		if (!VbGetSystemPropertyString("hwid", hwid, sizeof(hwid))) {
			char fwtype_buf[VB_MAX_STRING_PROPERTY];
			const char *fwtype = VbGetSystemPropertyString(
			    "mainfw_type", fwtype_buf, sizeof(fwtype_buf));
			if (fwtype && !strcasecmp(fwtype, "nonchrome")) {
				value = 1;
			}
		} else {
			value = 0;
		}
	}

	return value;
}

const char *VbGetSystemPropertyString(const char *name, char *dest,
                                      size_t size)
{
	/* Check architecture-dependent properties first */
	if (VbGetArchPropertyString(name, dest, size))
		return dest;

	if (!strcasecmp(name,"kernkey_vfy")) {
		switch(GetVdatInt(VDAT_INT_KERNEL_KEY_VERIFIED)) {
			case 0:
				return "hash";
			case 1:
				return "sig";
			default:
				return NULL;
		}
	} else if (!strcasecmp(name, "mainfw_act")) {
		return GetVdatString(dest, size, VDAT_STRING_MAINFW_ACT);
	} else if (!strcasecmp(name, "vdat_timers")) {
		return GetVdatString(dest, size, VDAT_STRING_TIMERS);
	} else if (!strcasecmp(name, "vdat_lfdebug")) {
		return GetVdatString(dest, size,
				     VDAT_STRING_LOAD_FIRMWARE_DEBUG);
	} else if (!strcasecmp(name, "vdat_lkdebug")) {
		return GetVdatString(dest, size, VDAT_STRING_LOAD_KERNEL_DEBUG);
	} else if (!strcasecmp(name, "fw_try_next")) {
		return vb2_get_nv_storage(VB2_NV_TRY_NEXT) ? "B" : "A";
	} else if (!strcasecmp(name, "fw_tried")) {
		return vb2_get_nv_storage(VB2_NV_FW_TRIED) ? "B" : "A";
	} else if (!strcasecmp(name, "fw_result")) {
		int v = vb2_get_nv_storage(VB2_NV_FW_RESULT);
		if (v < ARRAY_SIZE(fw_results))
			return fw_results[v];
		else
			return "unknown";
	} else if (!strcasecmp(name, "fw_prev_tried")) {
		return vb2_get_nv_storage(VB2_NV_FW_PREV_TRIED) ? "B" : "A";
	} else if (!strcasecmp(name, "fw_prev_result")) {
		int v = vb2_get_nv_storage(VB2_NV_FW_PREV_RESULT);
		if (v < ARRAY_SIZE(fw_results))
			return fw_results[v];
		else
			return "unknown";
	} else if (!strcasecmp(name,"dev_default_boot")) {
		int v = vb2_get_nv_storage(VB2_NV_DEV_DEFAULT_BOOT);
		if (v < ARRAY_SIZE(default_boot))
			return default_boot[v];
		else
			return "unknown";
	}

	return NULL;
}


int VbSetSystemPropertyInt(const char *name, int value)
{
	/* Check architecture-dependent properties first */

	if (0 == VbSetArchPropertyInt(name, value))
		return 0;

	/* NV storage values */
	if (!strcasecmp(name,"nvram_cleared")) {
		/* Can only clear this flag; it's set inside the NV storage
		 * library. */
		return vb2_set_nv_storage(VB2_NV_KERNEL_SETTINGS_RESET, 0);
	} else if (!strcasecmp(name,"recovery_request")) {
		return vb2_set_nv_storage(VB2_NV_RECOVERY_REQUEST, value);
	} else if (!strcasecmp(name,"recovery_subcode")) {
		return vb2_set_nv_storage(VB2_NV_RECOVERY_SUBCODE, value);
	} else if (!strcasecmp(name,"dbg_reset")) {
		return vb2_set_nv_storage(VB2_NV_DEBUG_RESET_MODE, value);
	} else if (!strcasecmp(name,"disable_dev_request")) {
		return vb2_set_nv_storage(VB2_NV_DISABLE_DEV_REQUEST, value);
	} else if (!strcasecmp(name,"clear_tpm_owner_request")) {
		return vb2_set_nv_storage(VB2_NV_CLEAR_TPM_OWNER_REQUEST, value);
	} else if (!strcasecmp(name,"clear_tpm_owner_done")) {
		/* Can only clear this flag; it's set by firmware. */
		return vb2_set_nv_storage(VB2_NV_CLEAR_TPM_OWNER_DONE, 0);
	} else if (!strcasecmp(name,"fwb_tries") ||
		   !strcasecmp(name,"fw_try_count")) {
		return vb2_set_nv_storage(VB2_NV_TRY_COUNT, value);
	} else if (!strcasecmp(name,"oprom_needed")) {
		return vb2_set_nv_storage(VB2_NV_OPROM_NEEDED, value);
	} else if (!strcasecmp(name,"wipeout_request")) {
		/* Can only clear this flag, set only by firmware. */
		return vb2_set_nv_storage(VB2_NV_REQ_WIPEOUT, 0);
	} else if (!strcasecmp(name,"backup_nvram_request")) {
		/* Best-effort only, since it requires firmware and TPM
		 * support. */
		return vb2_set_nv_storage(VB2_NV_BACKUP_NVRAM_REQUEST, value);
	} else if (!strcasecmp(name,"fwupdate_tries")) {
		int kern_nv = vb2_get_nv_storage(VB2_NV_KERNEL_FIELD);
		if (kern_nv == -1)
			return -1;
		kern_nv &= ~KERN_NV_FWUPDATE_TRIES_MASK;
		kern_nv |= (value & KERN_NV_FWUPDATE_TRIES_MASK);
		return vb2_set_nv_storage_with_backup(VB2_NV_KERNEL_FIELD,
						      kern_nv);
	} else if (!strcasecmp(name,"block_devmode")) {
		int kern_nv = vb2_get_nv_storage(VB2_NV_KERNEL_FIELD);
		if (kern_nv == -1)
			return -1;
		kern_nv &= ~KERN_NV_BLOCK_DEVMODE_FLAG;
		if (value)
			kern_nv |= KERN_NV_BLOCK_DEVMODE_FLAG;
		return vb2_set_nv_storage_with_backup(VB2_NV_KERNEL_FIELD,
						      kern_nv);
	} else if (!strcasecmp(name,"tpm_attack")) {
		/* This value should only be read and cleared, but we allow
		 * setting it to 1 for testing. */
		int kern_nv = vb2_get_nv_storage(VB2_NV_KERNEL_FIELD);
		if (kern_nv == -1)
			return -1;
		kern_nv &= ~KERN_NV_TPM_ATTACK_FLAG;
		if (value)
			kern_nv |= KERN_NV_TPM_ATTACK_FLAG;
		return vb2_set_nv_storage_with_backup(
		    VB2_NV_KERNEL_FIELD, kern_nv);
	} else if (!strcasecmp(name,"loc_idx")) {
		return vb2_set_nv_storage_with_backup(
		    VB2_NV_LOCALIZATION_INDEX,
						 value);
	} else if (!strcasecmp(name,"dev_boot_usb")) {
		return vb2_set_nv_storage_with_backup(
		    VB2_NV_DEV_BOOT_USB, value);
	} else if (!strcasecmp(name,"dev_boot_legacy")) {
		return vb2_set_nv_storage_with_backup(
		    VB2_NV_DEV_BOOT_LEGACY, value);
	} else if (!strcasecmp(name,"dev_boot_signed_only")) {
		return vb2_set_nv_storage_with_backup(
		    VB2_NV_DEV_BOOT_SIGNED_ONLY, value);
	} else if (!strcasecmp(name,"dev_boot_fastboot_full_cap")) {
		return vb2_set_nv_storage_with_backup(
		    VB2_NV_DEV_BOOT_FASTBOOT_FULL_CAP, value);
	} else if (!strcasecmp(name, "fastboot_unlock_in_fw")) {
		return vb2_set_nv_storage_with_backup(
		    VB2_NV_FASTBOOT_UNLOCK_IN_FW, value);
	} else if (!strcasecmp(name, "dev_enable_udc")) {
		return vb2_set_nv_storage_with_backup(
		    VB2_NV_DEV_ENABLE_UDC, value);
	} else if (!strcasecmp(name, "boot_on_ac_detect")) {
		return vb2_set_nv_storage_with_backup(
		    VB2_NV_BOOT_ON_AC_DETECT, value);
	} else if (!strcasecmp(name, "try_ro_sync")) {
		return vb2_set_nv_storage_with_backup(
		    VB2_NV_TRY_RO_SYNC, value);
	} else if (!strcasecmp(name, "battery_cutoff_request")) {
		return vb2_set_nv_storage(VB2_NV_BATTERY_CUTOFF_REQUEST, value);
	} else if (!strcasecmp(name,"kernel_max_rollforward")) {
		return vb2_set_nv_storage(VB2_NV_KERNEL_MAX_ROLLFORWARD, value);
	}

	return -1;
}

int VbSetSystemPropertyString(const char* name, const char* value)
{
	/* Chain to architecture-dependent properties */
	if (0 == VbSetArchPropertyString(name, value))
		return 0;

	if (!strcasecmp(name, "fw_try_next")) {
		if (!strcasecmp(value, "A"))
			return vb2_set_nv_storage(VB2_NV_TRY_NEXT, 0);
		else if (!strcasecmp(value, "B"))
			return vb2_set_nv_storage(VB2_NV_TRY_NEXT, 1);
		else
			return -1;

	} else if (!strcasecmp(name, "fw_result")) {
		int i;

		for (i = 0; i < ARRAY_SIZE(fw_results); i++) {
			if (!strcasecmp(value, fw_results[i]))
				return vb2_set_nv_storage(VB2_NV_FW_RESULT, i);
		}
		return -1;
	} else if (!strcasecmp(name, "dev_default_boot")) {
		int i;

		for (i = 0; i < ARRAY_SIZE(default_boot); i++) {
			if (!strcasecmp(value, default_boot[i]))
				return vb2_set_nv_storage(
				    VB2_NV_DEV_DEFAULT_BOOT, i);
		}
		return -1;
	}

	return -1;
}

static int InAndroid(void)
{
	int fd;
	struct stat s;
	int retval = 0;

	/*
	 * In Android, mosys utility located in /system/bin check if file
	 * exists.  Using fstat because for some reason, stat() was seg
	 * faulting in Android
	 */
	fd = open(MOSYS_ANDROID_PATH, O_RDONLY);
	if (fd != -1) {
		if (fstat(fd, &s) == 0)
			retval = 1;
		close(fd);
	}
	return retval;
}

static int ExecuteMosys(char * const argv[], char *buf, size_t bufsize)
{
	int status, mosys_to_crossystem[2];
	pid_t pid;
	ssize_t n;

	if (pipe(mosys_to_crossystem) < 0) {
		fprintf(stderr, "pipe() error\n");
		return -1;
	}

	if ((pid = fork()) < 0) {
		fprintf(stderr, "fork() error\n");
		close(mosys_to_crossystem[0]);
		close(mosys_to_crossystem[1]);
		return -1;
	} else if (!pid) {  /* Child */
		close(mosys_to_crossystem[0]);
		/* Redirect pipe's write-end to mosys' stdout */
		if (STDOUT_FILENO != mosys_to_crossystem[1]) {
			if (dup2(mosys_to_crossystem[1], STDOUT_FILENO)
			    != STDOUT_FILENO) {
				fprintf(stderr, "stdout dup2() failed (mosys)\n");
				close(mosys_to_crossystem[1]);
				exit(1);
			}
		}
		/* Execute mosys */
		execv(InAndroid() ? MOSYS_ANDROID_PATH : MOSYS_CROS_PATH, argv);
		/* We shouldn't be here; exit now! */
		fprintf(stderr, "execv() of mosys failed\n");
		close(mosys_to_crossystem[1]);
		exit(1);
	} else {  /* Parent */
		close(mosys_to_crossystem[1]);
		if (bufsize) {
			bufsize--;  /* Reserve 1 byte for '\0' */
			while ((n = read(mosys_to_crossystem[0],
					 buf, bufsize)) > 0) {
				buf += n;
				bufsize -= n;
			}
			*buf = '\0';
		} else {
			n = 0;
		}
		close(mosys_to_crossystem[0]);
		if (n < 0)
			fprintf(stderr, "read() error on output from mosys\n");
		if (waitpid(pid, &status, 0) < 0 || status) {
			fprintf(stderr, "waitpid() or mosys error\n");
			return -1;
		}
		if (n < 0)
			return -1;
	}
	return 0;
}

int vb2_read_nv_storage_mosys(struct vb2_context *ctx)
{
	/* Reserve extra 32 bytes */
	char hexstring[VB2_NVDATA_SIZE_V2 * 2 + 32];
	/*
	 * TODO(rspangler): mosys doesn't know how to read anything but 16-byte
	 * records yet.  When it grows a command line option to do that, call
	 * it here when needed.
	 *
	 * It's possible mosys won't need that.  For example, if if examines
	 * the header byte to determine the records size, or if it calls back
	 * to crossystem to read the VBSD flag.
	 */
	char * const argv[] = {
		InAndroid() ? MOSYS_ANDROID_PATH : MOSYS_CROS_PATH,
		"nvram", "vboot", "read", NULL
	};
	char hexdigit[3];
	const int nvsize = vb2_nv_get_size(ctx);
	int i;

	if (ExecuteMosys(argv, hexstring, sizeof(hexstring)))
		return -1;
	if (strlen(hexstring) < 2 * nvsize) {
		fprintf(stderr, "mosys returned hex nvdata size %d"
			" (need %d)\n", (int)strlen(hexstring), 2 * nvsize);
		return -1;
	}
	hexdigit[2] = '\0';
	for (i = 0; i < nvsize; i++) {
		hexdigit[0] = hexstring[i * 2];
		hexdigit[1] = hexstring[i * 2 + 1];
		ctx->nvdata[i] = strtol(hexdigit, NULL, 16);
	}
	return 0;
}

int vb2_write_nv_storage_mosys(struct vb2_context *ctx)
{
	char hexstring[VB2_NVDATA_SIZE_V2 * 2 + 1];
	char * const argv[] = {
		InAndroid() ? MOSYS_ANDROID_PATH : MOSYS_CROS_PATH,
		"nvram", "vboot", "write", hexstring, NULL
	};
	const int nvsize = vb2_nv_get_size(ctx);
	int i;

	for (i = 0; i < nvsize; i++)
		snprintf(hexstring + i * 2, 3, "%02x", ctx->nvdata[i]);
	hexstring[sizeof(hexstring) - 1] = '\0';
	if (ExecuteMosys(argv, NULL, 0))
		return -1;
	return 0;
}
