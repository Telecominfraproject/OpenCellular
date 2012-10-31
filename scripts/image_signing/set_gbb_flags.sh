#!/bin/sh
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This script can change GBB flags in system live firmware or a given image
# file.

SCRIPT_BASE="$(dirname "$0")"
. "$SCRIPT_BASE/common_minimal.sh"
load_shflags || exit 1

# DEFINE_string name default_value description flag
DEFINE_string file "" "Path to firmware image. Default to system firmware." "f"

# Globals
# ----------------------------------------------------------------------------
set -e

# Values from vboot_reference/firmware/include/gbb_header.h
GBBFLAGS_DESCRIPTION="
  Defined flags (some values may be not supported by all systems):

  GBB_FLAG_DEV_SCREEN_SHORT_DELAY     0x00000001
  GBB_FLAG_LOAD_OPTION_ROMS           0x00000002
  GBB_FLAG_ENABLE_ALTERNATE_OS        0x00000004
  GBB_FLAG_FORCE_DEV_SWITCH_ON        0x00000008
  GBB_FLAG_FORCE_DEV_BOOT_USB         0x00000010
  GBB_FLAG_DISABLE_FW_ROLLBACK_CHECK  0x00000020
  GBB_FLAG_ENTER_TRIGGERS_TONORM      0x00000040
  GBB_FLAG_FORCE_DEV_BOOT_LEGACY      0x00000080
  GBB_FLAG_FAFT_KEY_OVERIDE           0x00000100

  To get a developer-friendly device, try 0x11 (short_delay + boot_usb).
  For factory-related tests (always DEV), try 0x39.
"

FLAGS_HELP="Changes ChromeOS Firmware GBB Flags value.

  Usage: $0 [option_flags] GBB_FLAGS_VALUE
  $GBBFLAGS_DESCRIPTION"

FLASHROM_COMMON_OPT="-p internal:bus=spi"
FLASHROM_READ_OPT="$FLASHROM_COMMON_OPT -i GBB -r"
FLASHROM_WRITE_OPT="$FLASHROM_COMMON_OPT -i GBB --fast-verify -w"

# Main
# ----------------------------------------------------------------------------
main() {
  if [ "$#" != "1" ]; then
    flags_help
    exit 1
  fi

  local value="$(($1))"
  local image_file="$FLAGS_file"

  if [ -z "$FLAGS_file" ]; then
    image_file="$(make_temp_file)"
    flashrom $FLASHROM_READ_OPT "$image_file"
  fi

  # Process file
  local old_value="$(gbb_utility -g --flags "$image_file")"
  printf "Setting GBB flags from %s to 0x%x.." "$old_value" "$value" >&2
  gbb_utility -s --flags="$value" "$image_file"

  if [ -z "$FLAGS_file" ]; then
    flashrom $FLASHROM_WRITE_OPT "$image_file"
  fi
}

# Parse command line
FLAGS "$@" || exit 1
ORIGINAL_PARAMS="$@"
eval set -- "$FLAGS_ARGV"

main "$@"
