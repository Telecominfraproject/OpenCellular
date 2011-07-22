#!/bin/sh
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This script can change key (usually developer keys) in a firmware binary
# image or system live firmware (EEPROM), and assign proper HWID, BMPFV as well.

SCRIPT_BASE="$(dirname "$0")"
. "$SCRIPT_BASE/common_minimal.sh"
load_shflags || exit 1

# Constants used by DEFINE_*
VBOOT_BASE='/usr/share/vboot'
DEFAULT_KEYS_FOLDER="$VBOOT_BASE/devkeys"
DEFAULT_BMPFV_FILE="<auto>"
DEFAULT_BACKUP_FOLDER='/mnt/stateful_partition/backups'
DEFAULT_FIRMWARE_UPDATER='/usr/sbin/chromeos-firmwareupdate'

# DEFINE_string name default_value description flag
DEFINE_string from "" "Path of input file (empty for system live firmware)" "f"
DEFINE_string to "" "Path of output file (empty for system live firmware)" "t"
DEFINE_string keys "$DEFAULT_KEYS_FOLDER" "Path to folder of dev keys" "k"
DEFINE_string bmpfv "$DEFAULT_BMPFV_FILE" \
  "Path to the new bitmaps, <auto> to extract from system, empty to keep." ""
DEFINE_boolean force_backup \
  $FLAGS_TRUE "Create backup even if source is not live" ""
DEFINE_string backup_dir \
  "$DEFAULT_BACKUP_FOLDER" "Path of directory to store firmware backups" ""

# Parse command line
FLAGS "$@" || exit 1
eval set -- "$FLAGS_ARGV"

# Globals
# ----------------------------------------------------------------------------
set -e

# the image we are (temporary) working with
IMAGE="$(make_temp_file)"

# a log file to keep the output results of executed command
EXEC_LOG="$(make_temp_file)"

# Functions
# ----------------------------------------------------------------------------

# Disables write protection status registers
disable_write_protection() {
  # No need to change WP status in file mode
  if [ -n "$FLAGS_to" ]; then
    return $FLAGS_TRUE
  fi

  # --wp-disable command may return success even if WP is still enabled,
  # so we should use --wp-status to verify the results.
  echo "Disabling system software write protection status..."
  (flashrom --wp-disable && flashrom --wp-status) 2>&1 |
    tee "$EXEC_LOG" |
    grep -q '^WP: .* is disabled\.$'
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
    # TODO(hungte) we can enable partial write to make this faster
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
  # Also there's some length limitation in chromeos_acpi/HWID, so
  # a "{GUID} DEV" will become "{GUID} " in that case.

  if [ "$hwid" != "$hwid_no_dev" ]; then
    hwid="$hwid_no_dev"
  fi
  local hwid_dev="$hwid DEV"
  debug_msg "echo_dev_hwid: [$1] -> [$hwid_dev]"
  echo "$hwid_dev"
}

# Explores compatible firmware bitmaps
explore_bmpfv() {
  local tmp_folder=""

  if [ -s "$DEFAULT_FIRMWARE_UPDATER" ]; then
    # try to extract from built-in firmware updater
    debug_msg "found default firmware updater, trying to fetch bitmap..."
    tmp_folder=$("$DEFAULT_FIRMWARE_UPDATER" --sb_extract | sed "s'[^/]*''")
    debug_msg "updater resources extrated to: $tmp_folder"

    if [ -d "$tmp_folder" -a -s "$tmp_folder/bios.bin" ]; then
      new_bmpfv="$tmp_folder/bmpfv.bin"
      echo "$new_bmpfv"
      gbb_utility --bmpfv="$new_bmpfv" "$tmp_folder/bios.bin" >/dev/null 2>&1
    else
      debug_msg "failed to find valid BIOS image file."
    fi
  else
    debug_msg "no firmware updater in system. not changing bitmaps."
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
  local dev_firmware_keyblock="$FLAGS_keys/dev_firmware.keyblock"
  local dev_firmware_prvkey="$FLAGS_keys/dev_firmware_data_key.vbprivk"
  local kernel_sub_pubkey="$FLAGS_keys/kernel_subkey.vbpubk"
  local new_bmpfv="$FLAGS_bmpfv"
  local is_from_live=0
  local backup_image=
  local opt_bmpfv=""

  if [ "$new_bmpfv" = "$DEFAULT_BMPFV_FILE" ]; then
    new_bmpfv=$(explore_bmpfv) &&
      debug_msg "Using bitmaps from $new_bmpfv"
  fi

  debug_msg "Prerequisite check"
  ensure_files_exist \
    "$root_pubkey" \
    "$recovery_pubkey" \
    "$firmware_keyblock" \
    "$firmware_prvkey" \
    "$kernel_sub_pubkey" ||
    exit 1

  if [ -n "$new_bmpfv" ]; then
    opt_bmpfv="--bmpfv=$new_bmpfv"
    ensure_files_exist "$new_bmpfv" || exit 1
  fi

  if [ -z "$FLAGS_from" ]; then
    is_from_live=1
  else
    ensure_files_exist "$FLAGS_from" || exit 1
  fi

  debug_msg "Checking software write protection status"
  disable_write_protection ||
    if is_debug_mode; then
      err_die "Failed to disable WP. Diagnose Message: $(cat "$EXEC_LOG")"
    else
      err_die "Write protection is still enabled. " \
              "Please verify that hardware write protection is disabled."
    fi

  debug_msg "Pulling image to $IMAGE"
  (read_image && [ -s "$IMAGE" ]) ||
    err_die "Failed to read image. Error message: $(cat "$EXEC_LOG")"

  debug_msg "Prepare to backup the file"
  if [ -n "$is_from_live" -o $FLAGS_force_backup = $FLAGS_TRUE ]; then
    backup_image="$(make_temp_file)"
    debug_msg "Creating backup file to $backup_image..."
    cp -f "$IMAGE" "$backup_image"
  fi

  # TODO(hungte) We can use vbutil_firmware to check if the current firmware is
  # valid so that we know keys and vbutil_firmware are all working fine.

  echo "Preparing new firmware image..."
  debug_msg "Extract current HWID and rootkey"
  local old_hwid
  old_hwid="$(gbb_utility --get --hwid "$IMAGE" 2>"$EXEC_LOG" |
              grep '^hardware_id:' |
              sed 's/^hardware_id: //')"

  debug_msg "Decide new HWID"
  if [ -z "$old_hwid" ]; then
    err_die "Cannot find current HWID. (message: $(cat "$EXEC_LOG"))"
  fi
  local new_hwid="$(echo_dev_hwid "$old_hwid")"

  debug_msg "Replace GBB parts (gbb_utility allows changing on-the-fly)"
  gbb_utility --set \
    --hwid="$new_hwid" \
    --rootkey="$root_pubkey" \
    --recoverykey="$recovery_pubkey" \
    $opt_bmpfv \
    "$IMAGE" >"$EXEC_LOG" 2>&1 ||
    err_die "Failed to change GBB Data. (message: $(cat "$EXEC_LOG"))"

  debug_msg "Resign the firmware code (A/B) with new keys"
  local unsigned_image="$(make_temp_file)"
  cp -f "$IMAGE" "$unsigned_image"
  # TODO(hungte) derive kernel key and preamble flag from existing firmware
  "$SCRIPT_BASE/resign_firmwarefd.sh" \
    "$unsigned_image" \
    "$IMAGE" \
    "$firmware_prvkey" \
    "$firmware_keyblock" \
    "$dev_firmware_prvkey" \
    "$dev_firmware_keyblock" \
    "$kernel_sub_pubkey" >"$EXEC_LOG" 2>&1 ||
    err_die "Failed to re-sign firmware. (message: $(cat "$EXEC_LOG"))"
    if is_debug_mode; then
      cat "$EXEC_LOG"
    fi

  # TODO(hungte) compare if the image really needs to be changed.

  debug_msg "Check if we need to make backup file(s)"
  if [ -n "$backup_image" ]; then
    local backup_hwid_name="$(echo "$old_hwid" | sed 's/ /_/g')"
    local backup_date_time="$(date +'%Y%m%d_%H%M%S')"
    local backup_file_name="firmware_${backup_hwid_name}_${backup_date_time}.fd"
    local backup_file_path="$FLAGS_backup_dir/$backup_file_name"
    if mkdir -p "$FLAGS_backup_dir" &&
       cp -f "$backup_image" "$backup_file_path"; then
      true
    elif cp -f "$backup_image" "/tmp/$backup_file_name"; then
      backup_file_path="/tmp/$backup_file_name"
    else
      backup_file_path=''
    fi
    if [ -n "$backup_file_path" -a -s "$backup_file_path" ]; then
      # TODO(hungte) maybe we can wrap the flashrom by 'make_dev_firmware.sh -r'
      # so that the only command to remember would be make_dev_firmware.sh.
      echo "
      Backup of current firmware image is stored in:
        $backup_file_path
      Please copy the backup file to a safe place ASAP.

      To stop using devkeys and restore original firmware, execute command:
        flashrom -w [PATH_TO_BACKUP_IMAGE]
      Ex: flashrom -w $backup_file_path
      "
    else
      echo "WARNING: Cannot create file in $FLAGS_backup_dir... Ignore backups."
    fi
  fi

  # TODO(hungte) use vbutil_firmware to check if the new firmware is valid.
  # Or, do verification in resign_firmwarefd.sh and trust it.

  debug_msg "Write the image"
  write_image ||
    err_die "Failed to write image. Error message: $(cat "$EXEC_LOG")"

  debug_msg "Complete."
  if [ -z "$FLAGS_to" ]; then
    echo "Successfully changed firmware to Developer Keys. New HWID: $new_hwid"
  else
    echo "Firmware '$FLAGS_to' now uses Developer Keys. New HWID: $new_hwid"
  fi
}

main
