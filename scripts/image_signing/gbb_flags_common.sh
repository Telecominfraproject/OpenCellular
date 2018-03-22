#!/bin/sh
#
# Copyright 2017 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This script provides tools to read or change GBB flags on a live system.

SCRIPT_BASE="$(dirname "$0")"
. "${SCRIPT_BASE}/common_minimal.sh"
load_shflags || exit 1

# Globals
# ----------------------------------------------------------------------------

# Values from vboot_reference/firmware/include/gbb_header.h
GBBFLAGS_DESCRIPTION_PREFIX="
  Defined flags (some values may be not supported by all systems):

  "
GBBFLAGS_LIST="
  GBB_FLAG_DEV_SCREEN_SHORT_DELAY            0x00000001
  GBB_FLAG_LOAD_OPTION_ROMS                  0x00000002
  GBB_FLAG_ENABLE_ALTERNATE_OS               0x00000004
  GBB_FLAG_FORCE_DEV_SWITCH_ON               0x00000008
  GBB_FLAG_FORCE_DEV_BOOT_USB                0x00000010
  GBB_FLAG_DISABLE_FW_ROLLBACK_CHECK         0x00000020
  GBB_FLAG_ENTER_TRIGGERS_TONORM             0x00000040
  GBB_FLAG_FORCE_DEV_BOOT_LEGACY             0x00000080
  GBB_FLAG_FAFT_KEY_OVERIDE                  0x00000100
  GBB_FLAG_DISABLE_EC_SOFTWARE_SYNC          0x00000200
  GBB_FLAG_DEFAULT_DEV_BOOT_LEGACY           0x00000400
  GBB_FLAG_DISABLE_PD_SOFTWARE_SYNC          0x00000800
  GBB_FLAG_DISABLE_LID_SHUTDOWN              0x00001000
  GBB_FLAG_FORCE_DEV_BOOT_FASTBOOT_FULL_CAP  0x00002000
  GBB_FLAG_FORCE_MANUAL_RECOVERY             0x00004000
  GBB_FLAG_DISABLE_FWMP                      0x00008000
  "

GBBFLAGS_DESCRIPTION_SUFFIX="
  To get a developer-friendly device, try 0x11 (short_delay + boot_usb).
  For factory-related tests (always DEV), try 0x39.
  For early development (disable EC/PD software sync), try 0xa39.
  "
GBBFLAGS_DESCRIPTION="${GBBFLAGS_DESCRIPTION_PREFIX}${GBBFLAGS_LIST}"
GBBFLAGS_DESCRIPTION="${GBBFLAGS_DESCRIPTION}${GBBFLAGS_DESCRIPTION_SUFFIX}"

FLAGS_HELP="Manages Chrome OS Firmware GBB Flags value.

  Usage: $0 [option_flags] GBB_FLAGS_VALUE
  ${GBBFLAGS_DESCRIPTION}"

flashrom_read() {
  flashrom -p host -i GBB -r "$@"
}

flashrom_write() {
  flashrom -p host -i GBB --fast-verify -w "$@"
}
