/* Copyright 2017 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 *
 * Private declarations for vboot_ui_menu.c. Defined here for easier testing.
 */

#ifndef VBOOT_REFERENCE_VBOOT_UI_MENU_PRIVATE_H_
#define VBOOT_REFERENCE_VBOOT_UI_MENU_PRIVATE_H_

#include "2api.h"
#include "vboot_api.h"

struct vb2_menu_item {
	const char *text;
	VbError_t (*action)(struct vb2_context *ctx);
};

struct vb2_menu {
	uint16_t size;
	uint16_t screen;
	struct vb2_menu_item *items;
};

typedef enum _VB_MENU {
	VB_MENU_DEV_WARNING,
	VB_MENU_DEV,
	VB_MENU_TO_NORM,
	VB_MENU_TO_DEV,
	VB_MENU_LANGUAGES,
	VB_MENU_OPTIONS,
	VB_MENU_RECOVERY_INSERT,
	VB_MENU_RECOVERY_NO_GOOD,
	VB_MENU_RECOVERY_BROKEN,
	VB_MENU_TO_NORM_CONFIRMED,
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

typedef enum _VB_OPTIONS_MENU {
	VB_OPTIONS_DBG_INFO,
	VB_OPTIONS_CANCEL,
	VB_OPTIONS_POWER_OFF,
	VB_OPTIONS_LANGUAGE,
	VB_OPTIONS_COUNT,
} VB_OPTIONS_MENU;

#endif
