#!/bin/sh
#
# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This script can change key (usually developer keys) in a firmware binary
# image or system live firmware, and assign proper HWID, BMPFV as well.

SCRIPT_BASE="$(dirname "$0")"
. "$SCRIPT_BASE/common.sh"

# Constants used by DEFINE_*
DEFAULT_BACKUP_FOLDER='/mnt/stateful_partition/backups'

# DEFINE_string name default_value description flag
DEFINE_string from "" "Path of input file (empty for system live firmware)" "f"
DEFINE_string to "" "Path of output file (empty for system live firmware)" "t"
DEFINE_string keys "$SCRIPT_BASE/keys" "Path of folder of dev keys" "k"
DEFINE_string bmpfv "$SCRIPT_BASE/rsrc/bmpfv.bin" "Path to the new bitmap FV" ""
DEFINE_boolean force_backup \
  $FLAGS_TRUE "Create backup even if source is not live" ""
DEFINE_string backup_dir \
  "$DEFAULT_BACKUP_FOLDER" "Path of directory to store firmware backups" ""
DEFINE_boolean debug $FLAGS_FALSE "Provide debug messages" "d"

# Parse command line
FLAGS "$@" || exit 1
eval set -- "$FLAGS_ARGV"

# Globals
# ----------------------------------------------------------------------------
set -e

# the image we are (temporary) working with
IMAGE=$(make_temp_file)

# a log file to keep the output results of executed command
EXEC_LOG=$(make_temp_file)

# Functions
# ----------------------------------------------------------------------------
# Reports error message and exit(1)
err_die() {
  echo "ERROR: $*" 1>&2
  exit 1
}

# Returns true if we're running in debug mode
is_debug_mode() {
  [ "$FLAGS_debug" = $FLAGS_TRUE ]
}

# Prints messages (in parameters) in debug mode
debug_msg() {
  if is_debug_mode; then
    echo "DEBUG: $*" 1>&2
  fi
}

# Reads $IMAGE from $FLAGS_from
read_image() {
  if [ -z "$FLAGS_from" ]; then
    echo "Reading system live firmware..."
    if is_debug_mode; then
      flashrom -V -r "$IMAGE"
    else
      flashrom -r "$IMAGE" >"$EXEC_LOG" 2>&1
    fi
  else
    debug_msg "reading from file: $FLAGS_from"
    cp -f "$FLAGS_from" "$IMAGE"
  fi
}

# Writes $IMAGE to $FLAGS_to
write_image() {
  if [ -z "$FLAGS_to" ]; then
    echo "Writing system live firmware..."
    if is_debug_mode; then
      flashrom -V -w "$IMAGE"
    else
      flashrom -w "$IMAGE" >"$EXEC_LOG" 2>&1
    fi
  else
    debug_msg "writing to file: $FLAGS_to"
    cp -f "$IMAGE" "$FLAGS_to"
    chmod a+r "$FLAGS_to"
  fi
}

# Converts HWID from $1 to proper format with "DEV" extension
echo_dev_hwid() {
  local hwid="$1"
  local hwid_no_dev="${hwid% DEV}"

  # NOTE: Some DEV firmware image files may put GUID in HWID.
  # These are not officially supported and they will see "{GUID} DEV".

  if [ "$hwid" != "$hwid_no_dev" ]; then
    hwid="$hwid_no_dev"
  fi
  local hwid_dev="$hwid DEV"
  debug_msg "echo_dev_hwid: [$1] -> [$hwid_dev]"
  echo "$hwid_dev"
}

# Checks if the files given by parameters all exist.
check_exist_or_die() {
  local file is_success=$FLAGS_TRUE
  for file in "$@"; do
    if [ ! -s "$file" ]; then
      echo "ERROR: Cannot find required file: $file"
      is_success=$FLAGS_FALSE
    fi
  done

  if [ "$is_success" = $FLAGS_FALSE ]; then
    exit 1
  fi
}

# Main
# ----------------------------------------------------------------------------
main() {
  # Check parameters
  local root_pubkey="$FLAGS_keys/root_key.vbpubk"
  local recovery_pubkey="$FLAGS_keys/recovery_key.vbpubk"
  local firmware_keyblock="$FLAGS_keys/firmware.keyblock"
  local firmware_prvkey="$FLAGS_keys/firmware_data_key.vbprivk"
  local kernel_sub_pubkey="$FLAGS_keys/kernel_subkey.vbpubk"
  local new_bmpfv="$FLAGS_bmpfv"
  local is_from_live=0
  local backup_image=

  debug_msg "Pre-requisition check"
  check_exist_or_die \
    "$root_pubkey" \
    "$recovery_pubkey" \
    "$firmware_keyblock" \
    "$firmware_prvkey" \
    "$kernel_sub_pubkey" \
    "$new_bmpfv"

  if [ -z "$FLAGS_from" ]; then
    is_from_live=1
  else
    check_exist_or_die "$FLAGS_from"
  fi

  # TODO(hungte) check if GPIO.3 (WP) is enabled

  debug_msg "Pulling image to $IMAGE"
  (read_image && [ -s "$IMAGE" ]) ||
    err_die "Failed to read image. Error message: $(cat "$EXEC_LOG")"

  debug_msg "Prepare to backup the file"
  if [ -n "$is_from_live" -o $FLAGS_force_backup = $FLAGS_TRUE ]; then
    backup_image=$(make_temp_file)
    debug_msg "Creating backup file to $backup_image..."
    cp -f "$IMAGE" "$backup_image"
  fi

  # TODO(hungte) We can use vbutil_firmware to check if the current firmware is
  # valid, so that we can know both they key, vbutil_firmware are working fine.

  echo "Preparing new firmware image..."

  debug_msg "Extract current HWID and rootkey"
  local old_hwid
  old_hwid=$(gbb_utility --get --hwid "$IMAGE" 2>"$EXEC_LOG" |
             grep '^hardware_id:' |
             sed 's/^hardware_id: //')

  debug_msg "Decide new HWID"
  if [ -z "$old_hwid" ]; then
    err_die "Cannot find current HWID. (message: $(cat "$EXEC_LOG"))"
  fi
  local new_hwid=$(echo_dev_hwid "$old_hwid")

  debug_msg "Replace GBB parts (gbb_utility allows changing on-the-fly)"
  gbb_utility --set \
    --hwid="$new_hwid" \
    --bmpfv="$new_bmpfv" \
    --rootkey="$root_pubkey" \
    --recoverykey="$recovery_pubkey" \
    "$IMAGE" >"$EXEC_LOG" 2>&1 ||
    err_die "Failed to change GBB Data. (message: $(cat "$EXEC_LOG"))"

  debug_msg "Resign the firmware code (A/B) with new keys"
  local unsigned_image=$(make_temp_file)
  cp -f "$IMAGE" "$unsigned_image"
  "$SCRIPT_BASE/resign_firmwarefd.sh" \
    "$unsigned_image" \
    "$IMAGE" \
    "$firmware_prvkey" \
    "$firmware_keyblock" \
    "$kernel_sub_pubkey" >"$EXEC_LOG" 2>&1 ||
    err_die "Failed to re-sign firmware. (message: $(cat "$EXEC_LOG"))"

  # TODO(hungte) compare if the image really needs to be changed.

  debug_msg "Backup files when reading from system live image."
  if [ -n "$backup_image" ]; then
    local backup_hwid_name=$(echo "$old_hwid" | sed 's/ /_/g')
    local backup_date_time=$(date +'%Y%m%d_%H%M%S')
    local backup_file_name="firmware_${backup_hwid_name}_${backup_date_time}.fd"
    local backup_file_path="$FLAGS_backup_dir/$backup_file_name"
    if mkdir -p "$FLAGS_backup_dir" &&
       cp -f "$backup_image" "$backup_file_path"; then
      echo "Backup of current firmware image is stored in: $backup_file_path"
    else
      echo "Cannot create file in $FLAGS_backup_dir... Ignore backups."
    fi
  fi

  # TODO(hungte) use vbutil_firmware to check if the new firmware is valid.

  debug_msg "Write the image"
  write_image ||
    err_die "Failed to write image. Error message: $(cat "$EXEC_LOG")"

  debug_msg "Complete."
  if [ -z "$FLAGS_to" ]; then
    echo "Successfully changed firmware to Developer Keys. New HWID: $new_hwid"
  else
    echo "Firmware image '$FLAGS_to' now uses Developer Keys. HWID: $new_hwid"
  fi
}

main
