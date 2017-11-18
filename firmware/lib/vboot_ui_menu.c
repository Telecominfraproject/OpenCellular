/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * High-level firmware wrapper API - user interface for RW firmware
 */

#include "2sysincludes.h"
#include "2common.h"
#include "2misc.h"
#include "2nvstorage.h"
#include "2rsa.h"
#include "ec_sync.h"
#include "gbb_access.h"
#include "gbb_header.h"
#include "load_kernel_fw.h"
#include "region.h"
#include "rollback_index.h"
#include "utility.h"
#include "vb2_common.h"
#include "vboot_api.h"
#include "vboot_audio.h"
#include "vboot_common.h"
#include "vboot_display.h"
#include "vboot_kernel.h"
#include "vboot_nvstorage.h"

static void VbAllowUsbBootMenu(struct vb2_context *ctx)
{
	vb2_nv_set(ctx, VB2_NV_DEV_BOOT_USB, 1);
}

/**
 * Checks GBB flags against VbExIsShutdownRequested() shutdown request to
 * determine if a shutdown is required.
 *
 * Returns true if a shutdown is required and false if no shutdown is required.
 */
static int VbWantShutdownMenu(uint32_t gbb_flags)
{
	uint32_t shutdown_request = VbExIsShutdownRequested();

	/* If desired, ignore shutdown request due to lid closure. */
	if (gbb_flags & GBB_FLAG_DISABLE_LID_SHUTDOWN)
		shutdown_request &= ~VB_SHUTDOWN_REQUEST_LID_CLOSED;

	/*
	 * In detachables, disabling shutdown due to power button.
	 * We are using it for selection instead.
	 */
	shutdown_request &= ~VB_SHUTDOWN_REQUEST_POWER_BUTTON;

	return !!shutdown_request;
}

static void VbTryLegacyMenu(int allowed)
{
	if (!allowed)
		VB2_DEBUG("Legacy boot is disabled\n");
	else if (0 != RollbackKernelLock(0))
		VB2_DEBUG("Error locking kernel versions on legacy boot.\n");
	else
		VbExLegacy();	/* Will not return if successful */

	/* If legacy boot fails, beep and return to calling UI loop. */
	VbExBeep(120, 400);
	VbExSleepMs(120);
	VbExBeep(120, 400);
}

uint32_t VbTryUsbMenu(struct vb2_context *ctx, VbCommonParams *cparams)
{
	uint32_t retval = VbTryLoadKernel(ctx, cparams, VB_DISK_FLAG_REMOVABLE);
	if (VBERROR_SUCCESS == retval) {
		VB2_DEBUG("booting USB\n");
	} else {
		VB2_DEBUG("no kernel found on USB\n");
		VbExBeep(250, 200);
		VbExSleepMs(120);
		/*
		 * Clear recovery requests from failed
		 * kernel loading, so that powering off
		 * at this point doesn't put us into
		 * recovery mode.
		 */
		vb2_nv_set(ctx, VB2_NV_RECOVERY_REQUEST,
			   VBNV_RECOVERY_NOT_REQUESTED);
	}
	return retval;
}

static const char dev_disable_msg[] =
	"Developer mode is disabled on this device by system policy.\n"
	"For more information, see http://dev.chromium.org/chromium-os/fwmp\n"
	"\n";

typedef enum _VB_MENU {
	VB_MENU_DEV_WARNING,
	VB_MENU_DEV,
	VB_MENU_TO_NORM,
	VB_MENU_RECOVERY,
	VB_MENU_TO_DEV,
	VB_MENU_LANGUAGES,
	VB_MENU_RECOVERY_INSERT,
	VB_MENU_COUNT,
} VB_MENU;

typedef enum _VB_DEV_WARNING_MENU {
	VB_WARN_OPTIONS,
	VB_WARN_DBG_INFO,
	VB_WARN_ENABLE_VER,
	VB_WARN_POWER_OFF,
	VB_WARN_LANGUAGE,
	VB_WARN_COUNT,
} VB_DEV_WARNING_MENU;

typedef enum _VB_DEV_MENU {
	VB_DEV_NETWORK,
	VB_DEV_LEGACY,
	VB_DEV_USB,
	VB_DEV_DISK,
	VB_DEV_CANCEL,
	VB_DEV_POWER_OFF,
	VB_DEV_LANGUAGE,
	VB_DEV_COUNT,
} VB_DEV_MENU;

typedef enum _VB_TO_NORM_MENU {
	VB_TO_NORM_CONFIRM,
	VB_TO_NORM_CANCEL,
	VB_TO_NORM_POWER_OFF,
	VB_TO_NORM_LANGUAGE,
	VB_TO_NORM_COUNT,
} VB_TO_NORM_MENU;

typedef enum _VB_RECOVERY_MENU {
	VB_RECOVERY_TO_DEV,
	VB_RECOVERY_DBG_INFO,
	VB_RECOVERY_POWER_OFF,
	VB_RECOVERY_LANGUAGE,
	VB_RECOVERY_COUNT,
} VB_RECOVERY_MENU;

typedef enum _VB_TO_DEV_MENU {
	VB_TO_DEV_CONFIRM,
	VB_TO_DEV_CANCEL,
	VB_TO_DEV_POWER_OFF,
	VB_TO_DEV_LANGUAGE,
	VB_TO_DEV_COUNT,
} VB_TO_DEV_MENU;

// TODO: currently we're only supporting
// english.  Will need to somehow find mapping
// from language to localization index.
typedef enum _VB_LANGUAGES_MENU {
	VB_LANGUAGES_EN_US,
	VB_LANGUAGES_COUNT,
} VB_LANGUAGES_MENU;

static VB_MENU current_menu = VB_MENU_DEV_WARNING;
static VB_MENU prev_menu = VB_MENU_DEV_WARNING;
static int current_menu_idx = VB_WARN_POWER_OFF;
static int selected = 0;
static int disabled_idx_mask = 0;
static uint32_t default_boot = VB2_DEV_DEFAULT_BOOT_DISK;
static uint32_t disable_dev_boot = 0;

// TODO: add in consts
static char *dev_warning_menu[] = {
	"Developer Options\n",
	"Show Debug Info\n",
	"Enable Root Verification\n",
	"Power Off\n",
	"Language\n"
};

static char *dev_menu[] = {
	"Boot Network Image (not working yet)\n",
	"Boot Legacy BIOS\n",
	"Boot USB Image\n",
	"Boot Developer Image\n",
	"Cancel\n",
	"Power Off\n",
	"Language\n"
};

static char *to_normal_menu[] = {
	"Confirm Enabling Verified Boot\n",
	"Cancel\n",
	"Power Off\n",
	"Language\n"
};

static char *recovery_menu[] = {
	"Enable developer mode\n",
	"Show Debug Info\n",
	"Power Off\n",
	"Language\n"
};

static char *to_dev_menu[] = {
	"Confirm enabling developer mode\n",
	"Cancel\n",
	"Power Off\n",
	"Language\n"
};

static char *languages_menu[] = {
	"US English\n",
};

/**
 * Get the string array and size of current_menu.
 *
 * @param menu:	The current_menu
 * @param menu_array:	Pointer to the menu's string array.  If menu_array==NULL
 *			will not return string array.
 * @param size:	Size of menu's string array.
 * @return VBERROR_SUCCESS, or non-zero error code if error.
 */
void vb2_get_current_menu_size(VB_MENU menu, char ***menu_array,
				    uint32_t *size)
{
	char **temp_menu = NULL;

	switch(menu) {
	case VB_MENU_DEV_WARNING:
		*size = VB_WARN_COUNT;
		temp_menu = dev_warning_menu;
		break;
	case VB_MENU_DEV:
		*size = VB_DEV_COUNT;
		temp_menu = dev_menu;
		break;
	case VB_MENU_TO_NORM:
		*size = VB_TO_NORM_COUNT;
		temp_menu = to_normal_menu;
		break;
	case VB_MENU_RECOVERY:
		*size = VB_RECOVERY_COUNT;
		temp_menu = recovery_menu;
		break;
	case VB_MENU_TO_DEV:
		*size = VB_TO_DEV_COUNT;
		temp_menu = to_dev_menu;
		break;
	case VB_MENU_LANGUAGES:
		*size = VB_LANGUAGES_COUNT;
		temp_menu = languages_menu;
		break;
	default:
		*size = 0;
	}
	if (menu_array)
		*menu_array = temp_menu;
}

/**
 * Print current_menu state, including selected entry.
 *
 * @return VBERROR_SUCCESS, or non-zero error code if error.
 */
VbError_t vb2_print_current_menu()
{
	uint32_t size = 0;
	int i = 0;
	static char **m = NULL;
	int highlight = 0;
	// TODO: We probably want to center this text.
	uint32_t xindex, yindex;

	// TODO: need to check for error code.
	vb2_get_current_menu_size(current_menu, &m, &size);

	/* Center block of text */
	VbExDisplayGetDimension(&xindex, &yindex);
	xindex = xindex/2 - strlen(m[0])/2;
	yindex = yindex/2 - size/2;

	// TODO: do clear screen here.
	/* Create menu string */
	for (i = 0; i < size; i++) {
		highlight = !!(current_menu_idx == i);
		VbExDisplayText(xindex, yindex, m[i], highlight);
		VB2_DEBUG("[%d,%d]: %s", xindex, yindex, m[i]);
		yindex++;
	}

	return VBERROR_SUCCESS;
}

/*
 * Static array for mapping current_menu to matching screen item in depthcharge.
 * Note that order here is important and needs to match that of items in
 * VB_MENU.
 */
static const uint32_t VB_MENU_TO_SCREEN_MAP[] = {
	VB_SCREEN_DEVELOPER_WARNING_MENU,
	VB_SCREEN_DEVELOPER_MENU,
	VB_SCREEN_DEVELOPER_TO_NORM_MENU,
	VB_SCREEN_RECOVERY_MENU,
	VB_SCREEN_RECOVERY_TO_DEV_MENU,
	VB_SCREEN_LANGUAGES_MENU,
	VB_SCREEN_RECOVERY_INSERT,
};

VbError_t vb2_draw_current_screen(struct vb2_context *ctx,
				  VbCommonParams *cparams) {
	uint32_t screen;
	if (current_menu < VB_MENU_COUNT)
		screen = VB_MENU_TO_SCREEN_MAP[current_menu];
	else
		return VBERROR_UNKNOWN;
	return VbDisplayMenu(ctx, cparams, screen, 0,
			     current_menu_idx, disabled_idx_mask);
}

/**
 * Set current_menu to new_current_menu
 *
 * @param new_current_menu:	new menu to set current_menu to
 * @param new_current_menu_idx: new idx to set current_menu_idx to
 * @return VBERROR_SUCCESS, or non-zero error code if error.
 */
VbError_t vb2_set_menu_items(VB_MENU new_current_menu,
			     int new_current_menu_idx)
{
	prev_menu = current_menu;
	current_menu = new_current_menu;
	current_menu_idx = new_current_menu_idx;
	/* Changing menus, so reset selected */
	selected = 0;

	return VBERROR_SUCCESS;
}

/**
 * This updates current_menu and current_menu_idx, as necessary
 *
 * @return VBERROR_SUCCESS, or non-zero error code if error.
 */
VbError_t vb2_update_menu(struct vb2_context *ctx)
{
	VbError_t ret = VBERROR_SUCCESS;
	VB_MENU next_menu_idx = current_menu_idx;
	uint32_t loc = vb2_nv_get(ctx, VB2_NV_LOCALIZATION_INDEX);
	switch(current_menu) {
	case VB_MENU_DEV_WARNING:
		switch(current_menu_idx) {
		case VB_WARN_OPTIONS:
			switch(default_boot) {
			case VBNV_DEV_DEFAULT_BOOT_DISK:
				next_menu_idx = VB_DEV_DISK;
				break;
			case VBNV_DEV_DEFAULT_BOOT_USB:
				next_menu_idx = VB_DEV_USB;
				break;
			case VBNV_DEV_DEFAULT_BOOT_LEGACY:
				next_menu_idx = VB_DEV_LEGACY;
				break;
			}

			/*
			 * 1. Select dev menu
			 * 2. Default to dev boot device
			 */
			vb2_set_menu_items(VB_MENU_DEV, next_menu_idx);
			break;
		case VB_WARN_DBG_INFO:
			/* Show debug info */
			break;
		case VB_WARN_ENABLE_VER:
			/*
			 * 1. Enable boot verification
			 * 2. Default to the confirm option
			 */
			vb2_set_menu_items(VB_MENU_TO_NORM,
					   VB_TO_NORM_CONFIRM);
			break;
		case VB_WARN_POWER_OFF:
			/* Power off machine */
			ret =  VBERROR_SHUTDOWN_REQUESTED;
			break;
		case VB_WARN_LANGUAGE:
			/* Languages */
			vb2_set_menu_items(VB_MENU_LANGUAGES,
					   loc);
			break;
		default:
			/* Invalid menu item.  Don't update anything. */
			break;
		}
		break;
	case VB_MENU_DEV:
		switch(current_menu_idx) {
		case VB_DEV_NETWORK:
			/* Boot network image */
			break;
		case VB_DEV_LEGACY:
			/* Boot legacy BIOS */
			break;
		case VB_DEV_USB:
			/* Boot USB image */
			break;
		case VB_DEV_DISK:
			/* Boot developer image */
			break;
		case VB_DEV_CANCEL:
			/*
			 * 1. Cancel (go back to developer warning menu)
			 * 2. Default to power off option.
			 */
			vb2_set_menu_items(VB_MENU_DEV_WARNING,
					   VB_WARN_POWER_OFF);
			break;
		case VB_DEV_POWER_OFF:
			/* Power off */
			ret = VBERROR_SHUTDOWN_REQUESTED;
			break;
		case VB_DEV_LANGUAGE:
			/* Language */
			vb2_set_menu_items(VB_MENU_LANGUAGES,
					   loc);
			break;
		default:
			/* Invalid menu item.  Don't update anything. */
			break;
		}
		break;
	case VB_MENU_TO_NORM:
		switch(current_menu_idx) {
		case VB_TO_NORM_CONFIRM:
			/* Confirm enabling verified boot */
			break;
		case VB_TO_NORM_CANCEL:
			/*
			 * 1. Cancel (go back to developer warning menu)
			 * 2. Default to power off
			 */
			vb2_set_menu_items(VB_MENU_DEV_WARNING,
					   VB_WARN_POWER_OFF);
			break;
		case VB_TO_NORM_POWER_OFF:
			/* Power off */
			ret = VBERROR_SHUTDOWN_REQUESTED;
			break;
		case VB_TO_NORM_LANGUAGE:
			/* Language */
			vb2_set_menu_items(VB_MENU_LANGUAGES,
					   loc);
			break;
		default:
			/* Invalid menu item.  Don't update anything */
			break;
		}
		break;
	case VB_MENU_RECOVERY_INSERT:
		vb2_set_menu_items(VB_MENU_RECOVERY,
				   VB_RECOVERY_POWER_OFF);
		break;
	case VB_MENU_RECOVERY:
		switch(current_menu_idx) {
		case VB_RECOVERY_TO_DEV:
			/*
			 * 1. Switch to TO_DEV menu
			 * 2. Default to cancel option
			 */
			vb2_set_menu_items(VB_MENU_TO_DEV,
					   VB_TO_DEV_CANCEL);
			break;
		case VB_RECOVERY_DBG_INFO:
			break;
		case VB_RECOVERY_POWER_OFF:
			ret = VBERROR_SHUTDOWN_REQUESTED;
			break;
		case VB_RECOVERY_LANGUAGE:
			vb2_set_menu_items(VB_MENU_LANGUAGES,
					   loc);
			break;
		default:
			/* Invalid menu item.  Don't update anything */
			break;
		}
		break;
	case VB_MENU_TO_DEV:
		switch(current_menu_idx) {
		case VB_TO_DEV_CONFIRM:
			/* Confirm enabling dev mode */
			break;
		case VB_TO_DEV_CANCEL:
			vb2_set_menu_items(VB_MENU_RECOVERY,
					   VB_RECOVERY_POWER_OFF);
			break;
		case VB_TO_DEV_POWER_OFF:
			ret = VBERROR_SHUTDOWN_REQUESTED;
			break;
		case VB_TO_DEV_LANGUAGE:
			vb2_set_menu_items(VB_MENU_LANGUAGES,
					   loc);
			break;
		default:
			/* Invalid menu item.  Don't update anything. */
			break;
		}
		break;
	case VB_MENU_LANGUAGES:
		/*
		 * Assume that we selected a language.  Go to previous
		 * menu.  Purposely bypassing vb2_set_menu_items() here
		 * because need to do in different order.
		 */
		current_menu = prev_menu;
		prev_menu = VB_MENU_LANGUAGES;
		/*
		 * default to power off index with the exception of
		 * TO_DEV and TO_NORM menus
		 */
		switch (current_menu) {
		case VB_MENU_DEV_WARNING:
			current_menu_idx = VB_WARN_POWER_OFF;
			break;
		case VB_MENU_DEV:
			current_menu_idx = VB_DEV_POWER_OFF;
			break;
		case VB_MENU_TO_NORM:
			current_menu_idx = VB_TO_NORM_CONFIRM;
			break;
		case VB_MENU_RECOVERY:
			current_menu_idx = VB_RECOVERY_POWER_OFF;
			break;
		case VB_MENU_TO_DEV:
			current_menu_idx = VB_TO_DEV_CANCEL;
			break;
		default:
			current_menu_idx = 0;
			break;
		}
		selected = current_menu_idx;
		break;
	default:
		VB2_DEBUG("Current Menu Invalid! 0x%x\n", current_menu_idx);
	}
	return ret;
}

/**
 * This updates the current locale to the current_menu_index
 * This function only does something in the VB_MENU_LANGUAGES menu
 * Otherwise it's a noop.
 *
 * @return VBERROR_SUCCESS
 */
VbError_t vb2_update_locale(struct vb2_context *ctx) {
	if (current_menu == VB_MENU_LANGUAGES) {
		vb2_nv_set(ctx, VB2_NV_LOCALIZATION_INDEX, current_menu_idx);
		vb2_nv_set(ctx, VB2_NV_BACKUP_NVRAM_REQUEST, 1);
#ifdef SAVE_LOCALE_IMMEDIATELY
		if (ctx->flags & VB2_CONTEXT_NVDATA_CHANGED) {
			VbExNvStorageWrite(ctx.nvdata);
			ctx.flags &= ~VB2_CONTEXT_NVDATA_CHANGED;
		}
#endif
	}
	return VBERROR_SUCCESS;
}

/**
 * Adjust the disabled_idx_mask based on current menu and settings.
 *
 * @param flags flag to check for dev/normal mode.
 * @return VBERROR_SUCCESS
 */
VbError_t vb2_set_disabled_idx_mask(uint32_t flags) {
	/* Disable "Enable Developer Mode" menu item */
	disabled_idx_mask = 0;
	if (current_menu == VB_MENU_RECOVERY &&
	    (flags & VBSD_BOOT_DEV_SWITCH_ON)) {
		disabled_idx_mask |= 1 << VB_RECOVERY_TO_DEV;
	}
	/* Disable Network Boot Option */
	if (current_menu == VB_MENU_DEV)
		disabled_idx_mask |= 1 << VB_DEV_NETWORK;
	/* Disable cancel option if enterprise disabled dev mode */
	if (current_menu == VB_MENU_TO_NORM &&
	    disable_dev_boot == 1)
		disabled_idx_mask |= 1 << VB_TO_NORM_CANCEL;
	return VBERROR_SUCCESS;
}

/**
 * Updates current_menu_idx upon an up/down key press, taking into
 * account disabled indices (from disabled_idx_mask).  The cursor
 * will not wrap, meaning that we block on the 0 or max index when
 * we hit the ends of the menu.
 *
 * @param  cparams  common params
 * @param  key      VOL_KEY_UP = increase index selection
 *                  VOL_KEY_DOWN = decrease index selection.
 *                  Every other key has no effect now.
 */
void vb2_update_selection(VbCommonParams *cparams, uint32_t key) {
	int idx;
	uint32_t menu_size;

	if (current_menu == VB_MENU_LANGUAGES) {
		VbGetLocalizationCount(cparams, &menu_size);
	} else {
		vb2_get_current_menu_size(current_menu,
					  NULL, &menu_size);
	}

	switch (key) {
	case VB_BUTTON_VOL_UP_SHORT_PRESS:
	case VB_KEY_UP:
		idx = current_menu_idx - 1;
		while (idx >= 0 &&
		       ((1 << idx) & disabled_idx_mask))
		  idx--;
		/* Only update if idx is valid */
		if (idx >= 0)
			current_menu_idx = idx;
		break;
	case VB_BUTTON_VOL_DOWN_SHORT_PRESS:
	case VB_KEY_DOWN:
		idx = current_menu_idx + 1;
		while (idx < menu_size &&
		       ((1 << idx) & disabled_idx_mask))
		  idx++;
		/* Only update if idx is valid */
		if (idx < menu_size)
			current_menu_idx = idx;
		break;
	default:
	  /* Do not update anything */
	  break;
	}
}

/**
 * Main function that handles developer warning menu functionality
 *
 * @param ctx		Vboot2 context
 * @param cparams	Vboot1 common params
 * @return VBERROR_SUCCESS, or non-zero error code if error.
 */
VbError_t vb2_developer_menu(struct vb2_context *ctx, VbCommonParams *cparams)
{
	GoogleBinaryBlockHeader *gbb = cparams->gbb;
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;

	uint32_t use_usb = 0;
	uint32_t use_legacy = 0;
	uint32_t ctrl_d_pressed = 0;

	VbAudioContext *audio = NULL;
	VbError_t ret;

	VB2_DEBUG("Entering\n");

	/* Check if USB booting is allowed */
	uint32_t allow_usb = vb2_nv_get(ctx, VB2_NV_DEV_BOOT_USB);
	uint32_t allow_legacy = vb2_nv_get(ctx, VB2_NV_DEV_BOOT_LEGACY);

	/* Check if the default is to boot using disk, usb, or legacy */
	default_boot = vb2_nv_get(ctx, VB2_NV_DEV_DEFAULT_BOOT);

	if(default_boot == VBNV_DEV_DEFAULT_BOOT_USB)
		use_usb = 1;
	if(default_boot == VBNV_DEV_DEFAULT_BOOT_LEGACY)
		use_legacy = 1;

	/* Handle GBB flag override */
	if (gbb->flags & GBB_FLAG_FORCE_DEV_BOOT_USB)
		allow_usb = 1;
	if (gbb->flags & GBB_FLAG_FORCE_DEV_BOOT_LEGACY)
		allow_legacy = 1;
	if (gbb->flags & GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY) {
		use_legacy = 1;
		use_usb = 0;
	}

	/* Handle FWMP override */
	uint32_t fwmp_flags = vb2_get_fwmp_flags();
	if (fwmp_flags & FWMP_DEV_ENABLE_USB)
		allow_usb = 1;
	if (fwmp_flags & FWMP_DEV_ENABLE_LEGACY)
		allow_legacy = 1;
	if (fwmp_flags & FWMP_DEV_DISABLE_BOOT) {
		if (gbb->flags & GBB_FLAG_FORCE_DEV_SWITCH_ON) {
			VB2_DEBUG("FWMP_DEV_DISABLE_BOOT rejected by"
				  "FORCE_DEV_SWITCH_ON\n");
		} else {
			disable_dev_boot = 1;
			VB2_DEBUG("dev_disable_boot is set.\n");

			/* If dev mode is disabled, only allow TONORM */
			current_menu = VB_MENU_TO_NORM;
			prev_menu = VB_MENU_TO_NORM;
			current_menu_idx = VB_TO_NORM_CONFIRM;
		}
	}

	vb2_set_disabled_idx_mask(shared->flags);
	/* Show the dev mode warning screen */
	vb2_draw_current_screen(ctx, cparams);

	/* Get audio/delay context */
	if (!disable_dev_boot)
		audio = VbAudioOpen(cparams);

	/* We'll loop until we finish the delay or are interrupted */
	do {
		uint32_t key;

		if (VbWantShutdownMenu(gbb->flags)) {
			VB2_DEBUG("shutdown requested!\n");
			if (audio)
				VbAudioClose(audio);
			return VBERROR_SHUTDOWN_REQUESTED;
		}

		/* Make sure user knows dev mode disabled */
		if (disable_dev_boot)
			VbExDisplayDebugInfo(dev_disable_msg);

		key = VbExKeyboardRead();
		switch (key) {
		case 0:
			/* Nothing pressed */
			break;
		case VB_BUTTON_VOL_DOWN_LONG_PRESS:
		case 0x04:
			/* Ctrl+D = dismiss warning; advance to timeout */
			if (disable_dev_boot)
				break;
			VB2_DEBUG("user pressed Ctrl+D; skip delay\n");
			ctrl_d_pressed = 1;
			goto fallout;
			break;
		case 0x0c:
			if (disable_dev_boot)
				break;
			VB2_DEBUG("user pressed Ctrl+L; Try legacy boot\n");
			VbTryLegacyMenu(allow_legacy);
			break;
		case VB_BUTTON_VOL_UP_LONG_PRESS:
		case 0x15:
			if (disable_dev_boot)
				break;
			/* Ctrl+U = try USB boot, or beep if failure */
			VB2_DEBUG("user pressed Ctrl+U; try USB\n");
			if (!allow_usb) {
				VB2_DEBUG("USB booting is disabled\n");
				VbExDisplayDebugInfo(
					"WARNING: Booting from external media "
					"(USB/SD) has not been enabled. Refer "
					"to the developer-mode documentation "
					"for details.\n");
				VbExBeep(120, 400);
				VbExSleepMs(120);
				VbExBeep(120, 400);
			} else {
				/*
				 * Clear the screen to show we get the Ctrl+U
				 * key press.
				 */
				VbDisplayScreen(ctx, cparams, VB_SCREEN_BLANK,
						0);
				if (VBERROR_SUCCESS ==
				    VbTryUsbMenu(ctx, cparams)) {
					if (audio)
						VbAudioClose(audio);
					return VBERROR_SUCCESS;
				} else {
					/* Show dev mode warning screen again */
					vb2_draw_current_screen(ctx, cparams);
				}
			}
			break;
		case VB_BUTTON_VOL_UP_SHORT_PRESS:
		case VB_KEY_UP:
			vb2_update_selection(cparams, key);
			vb2_draw_current_screen(ctx, cparams);
			/* reset 30 second timer */
			if (audio)
				audio = VbAudioOpen(cparams);
			break;
		case VB_BUTTON_VOL_DOWN_SHORT_PRESS:
		case VB_KEY_DOWN:
			vb2_update_selection(cparams, key);
			vb2_draw_current_screen(ctx, cparams);
			/* reset 30 second timer */
			if (audio)
				audio = VbAudioOpen(cparams);
			break;
		case VB_BUTTON_POWER_SHORT_PRESS:
		case '\r':
			selected = 1;

			/*
			 * Need to update locale before updating the menu or
			 * we'll lose the previous state
			 */
			vb2_update_locale(ctx);

			ret = vb2_update_menu(ctx);
			vb2_set_disabled_idx_mask(shared->flags);
			vb2_draw_current_screen(ctx, cparams);

			/* Probably shutting down */
			if (ret != VBERROR_SUCCESS) {
			  VB2_DEBUG("shutting down!\n");
			  return ret;
			}

			/* Nothing selected, skip everything else */
			if (selected == 0)
				break;

			/* All the actions associated with selection */

			/* Display debug information */
			if (current_menu == VB_MENU_DEV_WARNING &&
			    current_menu_idx == VB_WARN_DBG_INFO) {
				VbDisplayDebugInfo(ctx, cparams);
			}

			/* Boot Legacy mode */
			if (current_menu == VB_MENU_DEV &&
			    current_menu_idx == VB_DEV_LEGACY) {
				VB2_DEBUG("user pressed Ctrl+L; "
					  "Try legacy boot\n");
				VbTryLegacyMenu(allow_legacy);
			}

			/* USB boot, or beep if failure */
			if (current_menu == VB_MENU_DEV &&
			    current_menu_idx == VB_DEV_USB) {
				VB2_DEBUG("user pressed Ctrl+U; try USB\n");
				if (!allow_usb) {
					VB2_DEBUG("USB booting is disabled\n");
					VbExDisplayDebugInfo(
						"WARNING: Booting from external media "
						"(USB/SD) has not been enabled. Refer "
						"to the developer-mode documentation "
						"for details.\n");
					VbExBeep(120, 400);
					VbExSleepMs(120);
					VbExBeep(120, 400);
				} else {
					/*
					 * Clear the screen to show we get the
					 * Ctrl+U key press.
					 */
					VbDisplayScreen(ctx,
						cparams, VB_SCREEN_BLANK, 0);
					if (VBERROR_SUCCESS ==
					    VbTryUsbMenu(ctx, cparams)) {
						if (audio)
							VbAudioClose(audio);
						return VBERROR_SUCCESS;
					} else
						/*
						 * Show dev mode warning screen
						 * again
						 */
						vb2_draw_current_screen(ctx,
								       cparams);
				}
			}

			/* Boot developer mode: advance to timeout */
			if (current_menu == VB_MENU_DEV &&
			    current_menu_idx == VB_DEV_DISK) {
				VB2_DEBUG("user pressed Ctrl+D; skip delay\n");
				ctrl_d_pressed = 1;
				goto fallout;
			}

			/* Enabling verified boot */
			if (current_menu == VB_MENU_TO_NORM &&
			    current_menu_idx == VB_TO_NORM_CONFIRM) {
				if (gbb->flags & GBB_FLAG_FORCE_DEV_SWITCH_ON) {
					/*
					 * Throw error when user tries to
					 * confirm transition to normal
					 * mode if FORCE_DEV_SWITCH_ON
					 * is enabled.
					 */
					VB2_DEBUG("TONORM rejected by "
						  "FORCE_DEV_SWITCH_ON\n");
					VbExDisplayDebugInfo(
						"WARNING: TONORM prohibited by "
						"GBB FORCE_DEV_SWITCH_ON.\n\n");
					VbExBeep(120, 400);
				} else {
					/*
					 * See if we should disable
					 * virtual dev-mode switch.
					 */
					VB2_DEBUG("%s shared->flags=0x%x\n",
						  __func__, shared->flags);
					VB2_DEBUG("leaving dev-mode.\n");
					vb2_nv_set(ctx,
						   VB2_NV_DISABLE_DEV_REQUEST,
						   1);
					VbDisplayScreen(ctx,
						cparams,
						VB_SCREEN_TO_NORM_CONFIRMED,
						0);
					VbExSleepMs(5000);
					return VBERROR_REBOOT_REQUIRED;
				}
			}
			/* reset 30 second timer */
			if (audio)
				audio = VbAudioOpen(cparams);
			break;
		default:
			VB2_DEBUG("pressed key %d\n", key);
			break;
		}
	} while(VbAudioLooping(audio));

fallout:

	/* If defaulting to legacy boot, try that unless Ctrl+D was pressed */
	if (use_legacy && !ctrl_d_pressed) {
		VB2_DEBUG("defaulting to legacy\n");
		VbTryLegacyMenu(allow_legacy);
	}

	if ((use_usb && !ctrl_d_pressed) && allow_usb) {
		if (VBERROR_SUCCESS == VbTryUsbMenu(ctx, cparams)) {
			VbAudioClose(audio);
			return VBERROR_SUCCESS;
		}
	}

	/* Timeout or Ctrl+D; attempt loading from fixed disk */
	VB2_DEBUG("trying fixed disk\n");
	VbAudioClose(audio);
	return VbTryLoadKernel(ctx, cparams, VB_DISK_FLAG_FIXED);
}

VbError_t VbBootDeveloperMenu(struct vb2_context *ctx, VbCommonParams *cparams)
{
	VbError_t retval = vb2_developer_menu(ctx, cparams);
	VbDisplayScreen(ctx, cparams, VB_SCREEN_BLANK, 0);
	return retval;
}

/* Delay in recovery mode */
#define REC_DISK_DELAY       1000     /* Check disks every 1s */
#define REC_KEY_DELAY        20       /* Check keys every 20ms */
#define REC_MEDIA_INIT_DELAY 500      /* Check removable media every 500ms */

/**
 * Main function that handles recovery menu functionality
 *
 * @param ctx		Vboot2 context
 * @param cparams	Vboot1 common params
 * @return VBERROR_SUCCESS, or non-zero error code if error.
 */
static VbError_t recovery_ui(struct vb2_context *ctx, VbCommonParams *cparams)
{
	VbSharedDataHeader *shared =
		(VbSharedDataHeader *)cparams->shared_data_blob;
	uint32_t retval;
	uint32_t key;
	uint32_t key_flags;
	int i;
	VbError_t ret;

	VB2_DEBUG("start\n");

	if (!vb2_allow_recovery(shared->flags)) {
		/*
		 * We have to save the reason here so that it will survive
		 * coming up three-finger-salute. We're saving it in
		 * VBNV_RECOVERY_SUBCODE to avoid a recovery loop.
		 * If we save the reason in VBNV_RECOVERY_REQUEST, we will come
		 * back here, thus, we won't be able to give a user a chance to
		 * reboot to workaround a boot hiccup.
		 */
		VB2_DEBUG("saving recovery reason (%#x)\n",
			  shared->recovery_reason);
		vb2_nv_set(ctx, VB2_NV_RECOVERY_SUBCODE,
			   shared->recovery_reason);
		/*
		 * Commit NV now, because it won't get saved if the user forces
		 * manual recovery via the three-finger salute.
		 */
		vb2_nv_commit(ctx);

		VbDisplayScreen(ctx, cparams, VB_SCREEN_OS_BROKEN, 0);
		VB2_DEBUG("waiting for manual recovery\n");
		while (1) {
			key = VbExKeyboardRead();
			if (key == VB_BUTTON_POWER_SHORT_PRESS)
				return VBERROR_SHUTDOWN_REQUESTED;
			else {
				VbCheckDisplayKey(ctx, cparams, key);
				if (VbWantShutdownMenu(cparams->gbb->flags))
					return VBERROR_SHUTDOWN_REQUESTED;
			}
			VbExSleepMs(REC_KEY_DELAY);
		}
	}

	/* Loop and wait for a recovery image */
	VB2_DEBUG("waiting for a recovery image\n");

	/* Initialize menu to recovery menu. */
	current_menu = VB_MENU_RECOVERY_INSERT;
	prev_menu = VB_MENU_RECOVERY_INSERT;
	current_menu_idx = 0;

	while (1) {
		VB2_DEBUG("attempting to load kernel2\n");
		retval = VbTryLoadKernel(ctx, cparams, VB_DISK_FLAG_REMOVABLE);

		/*
		 * Clear recovery requests from failed kernel loading, since
		 * we're already in recovery mode.  Do this now, so that
		 * powering off after inserting an invalid disk doesn't leave
		 * us stuck in recovery mode.
		 */
		vb2_nv_set(ctx, VB2_NV_RECOVERY_REQUEST,
			   VBNV_RECOVERY_NOT_REQUESTED);

		if (VBERROR_SUCCESS == retval)
			break; /* Found a recovery kernel */

		vb2_set_disabled_idx_mask(shared->flags);

		if (current_menu != VB_MENU_RECOVERY ||
		    current_menu_idx != VB_RECOVERY_DBG_INFO) {
			if (retval == VBERROR_NO_DISK_FOUND)
				vb2_draw_current_screen(ctx, cparams);
			else
				VbDisplayScreen(ctx, cparams,
						VB_SCREEN_RECOVERY_NO_GOOD,
						0);
		}

		/*
		 * Scan keyboard more frequently than media, since x86
		 * platforms don't like to scan USB too rapidly.
		 */
		for (i = 0; i < REC_DISK_DELAY; i += REC_KEY_DELAY) {
			key = VbExKeyboardReadWithFlags(&key_flags);
			switch (key) {
			case 0:
				/* nothing pressed */
				break;
			case VB_KEY_UP:
			case VB_KEY_DOWN:
			case VB_BUTTON_VOL_UP_SHORT_PRESS:
			case VB_BUTTON_VOL_DOWN_SHORT_PRESS:
				/* User cannot use keyboard to enable dev mode.
				 * They need to use the volume buttons from a
				 * trusted source to navigate to the disable os
				 * verification menu item.  Beep so user knows
				 * that they're doing something wrong.
				 */
				if (current_menu == VB_MENU_TO_DEV &&
				    !(key_flags & VB_KEY_FLAG_TRUSTED_KEYBOARD)) {
					VbExBeep(120, 400);
					break;
				}

				vb2_update_selection(cparams, key);
				vb2_draw_current_screen(ctx, cparams);
				break;
			case VB_BUTTON_VOL_UP_DOWN_COMBO_PRESS:
				/*
				 * This is currently only being used to
				 * transition out of the initial insert
				 * graphic
				 */
				if (current_menu == VB_MENU_RECOVERY_INSERT) {
					ret = vb2_update_menu(ctx);
					if (ret != VBERROR_SUCCESS)
						return ret;
					vb2_set_disabled_idx_mask(shared->flags);
					vb2_draw_current_screen(ctx, cparams);
				}
				break;
			case VB_BUTTON_POWER_SHORT_PRESS:
			case '\r':
				selected = 1;

				/*
				 * If user hits power button in
				 * initial recovery screen (ie:
				 * because didn't really want to go
				 * there), power button will turn off
				 * device.
				 */
				if (current_menu == VB_MENU_RECOVERY_INSERT) {
					ret = VBERROR_SHUTDOWN_REQUESTED;
				} else {
					/*
					 * Need to update locale
					 * before updating the menu or
					 * we'll lose the previous state
					 */
					vb2_update_locale(ctx);

					ret = vb2_update_menu(ctx);

					vb2_set_disabled_idx_mask(shared->
								  flags);
				}
				if (current_menu != VB_MENU_RECOVERY ||
				     current_menu_idx != VB_RECOVERY_DBG_INFO) {
					/*
					 * Unfortunately we need this screen
					 * blanking to clear previous menus
					 * printed.
					 */
					if (retval == VBERROR_NO_DISK_FOUND)
						vb2_draw_current_screen(ctx,
							cparams);
					else
						VbDisplayScreen(ctx, cparams,
						VB_SCREEN_RECOVERY_NO_GOOD,
						0);
				}

				/* Probably shutting down */
				if (ret != VBERROR_SUCCESS) {
					VB2_DEBUG("update_menu - shutting down!\n");
					return ret;
				}

				/* Nothing selected, skip everything else. */
				if (selected == 0)
					break;

				/* Display debug information */
				if (current_menu == VB_MENU_RECOVERY &&
				    current_menu_idx == VB_RECOVERY_DBG_INFO) {
					VbDisplayDebugInfo(ctx, cparams);
				}

				/* Confirm going into developer mode */
				/*
				 * We might want to enter dev-mode from the Insert
				 * screen if all of the following are true:
				 *   - user pressed Ctrl-D
				 *   - we can honor the virtual dev switch
				 *   - not already in dev mode
				 *   - user forced recovery mode
				 */
				if (current_menu == VB_MENU_TO_DEV &&
				    current_menu_idx == 0 &&
				    shared->flags & VBSD_HONOR_VIRT_DEV_SWITCH &&
				    !(shared->flags & VBSD_BOOT_DEV_SWITCH_ON) &&
				    (shared->flags & VBSD_BOOT_REC_SWITCH_ON)) {
					if (!(shared->flags &
					      VBSD_BOOT_REC_SWITCH_VIRTUAL) &&
					    VbExGetSwitches(
						VB_INIT_FLAG_REC_BUTTON_PRESSED)) {
						/*
						 * Is the recovery button stuck?  In
						 * any case we don't like this.  Beep
						 * and ignore.
						 */
						VB2_DEBUG("^D but rec switch "
							  "is pressed\n");
						VbExBeep(120, 400);
						continue;
					}

					VB2_DEBUG("Enabling dev-mode...\n");
					if (TPM_SUCCESS != SetVirtualDevMode(1))
						return VBERROR_TPM_SET_BOOT_MODE_STATE;
					VB2_DEBUG("Reboot so it will take "
						  "effect\n");
					if (VbExGetSwitches
					    (VB_INIT_FLAG_ALLOW_USB_BOOT))
						VbAllowUsbBootMenu(ctx);
					return VBERROR_REBOOT_REQUIRED;
				}
			}
			if (VbWantShutdownMenu(cparams->gbb->flags))
				return VBERROR_SHUTDOWN_REQUESTED;
			VbExSleepMs(REC_KEY_DELAY);
		}
	}

	return VBERROR_SUCCESS;
}

VbError_t VbBootRecoveryMenu(struct vb2_context *ctx, VbCommonParams *cparams)
{
	VbError_t retval = recovery_ui(ctx, cparams);
	VbDisplayScreen(ctx, cparams, VB_SCREEN_BLANK, 0);
	return retval;
}
