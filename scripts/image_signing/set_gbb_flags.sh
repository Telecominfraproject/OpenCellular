#!/bin/sh
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This script can change GBB flags in system live firmware or a given image
# file.

SCRIPT_BASE="$(dirname "$0")"
. "${SCRIPT_BASE}/gbb_flags_common.sh"

# DEFINE_string name default_value description flag
DEFINE_string file "" "Path to firmware image. Default to system firmware." "f"
DEFINE_boolean check_wp ${FLAGS_TRUE} "Check write protection states first." ""

set -e

# Check write protection
# ----------------------------------------------------------------------------
check_write_protection() {
  local hw_wp="" sw_wp=""
  if ! crossystem "wpsw_boot?0"; then
    hw_wp="on"
  fi
  # Keep 'local' declaration split from assignment so return code is checked.
  local wp_states
  wp_states="$(flashrom -p host --wp-status 2>/dev/null | grep WP)"
  local wp_disabled="$(echo "${wp_states}" | grep "WP:.*is disabled.")"
  local wp_zero_len="$(echo "${wp_states}" | grep "WP:.*, len=0x00000000")"
  if [ -z "${wp_disabled}" -a -z "${wp_zero_len}" ]; then
    sw_wp="on"
  fi
  if [ -n "${hw_wp}" -a -n "${sw_wp}" ]; then
    return ${FLAGS_FALSE}
  fi
  return ${FLAGS_TRUE}
}

# Main
# ----------------------------------------------------------------------------
main() {
  if [ "$#" != "1" ]; then
    flags_help
    exit 1
  fi

  local value="$(($1))"
  local image_file="${FLAGS_file}"

  if [ -z "${FLAGS_file}" ]; then
    image_file="$(make_temp_file)"
    flashrom_read "${image_file}"
  fi

  # Process file
  # Keep 'local' declaration split from assignment so return code is checked.
  local old_value
  old_value="$(futility gbb -g --flags "${image_file}")"
  printf "Setting GBB flags from %s to 0x%x.." "${old_value}" "${value}"
  futility gbb -s --flags="${value}" "${image_file}"

  if [ -z "${FLAGS_file}" ]; then
    if [ "${FLAGS_check_wp}" = "${FLAGS_TRUE}" ]; then
      if ! check_write_protection; then
        echo ""
        echo "WARNING: System GBB Flags are NOT changed!!!"
        echo "ERROR: You must disable write protection before setting flags."
        exit 1
      fi
    fi
    flashrom_write "$image_file"
  fi
}

# Parse command line
FLAGS "$@" || exit 1
eval set -- "$FLAGS_ARGV"

main "$@"
