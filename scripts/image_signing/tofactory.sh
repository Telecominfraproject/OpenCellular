#!/bin/sh
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This script converts a Chrome OS device to a pre-factory-install state:
#  * Firmware write protect disabled
#  * H2O BIOS, with RO VPD area copied from the current BIOS
#  * Original EC firmware
#  * Blank SSD (no GPT)
#
# Minimal usage:
#   tofactory.sh -b H2OBIOS.bin -e ec_shellball.sh

SCRIPT_BASE="$(dirname "$0")"
. "$SCRIPT_BASE/common_minimal.sh"
load_shflags || exit 1

# Constants used by DEFINE_*
VBOOT_BASE='/usr/share/vboot'

# DEFINE_string name default_value description flag
DEFINE_string bios "" "Path of system firmware (BIOS) binary to write" "b"
DEFINE_string ec "" "Path of EC shellball to execute" "e"
DEFINE_string backup_dir "" "Path of directory in whoch to store backups" "k"
DEFINE_string asset_tag "unspecified_tag" \
  "Asset tag of device, used to name backups" "a"
DEFINE_string ssd "/dev/sda" "Path to SSD / target drive" "s"
DEFINE_boolean wipe_ssd $FLAGS_TRUE "Wipe SSD after firmware updates" ""
DEFINE_boolean nothing $FLAGS_FALSE \
  "Print commands but do not modify device" "n"

# Parse command line
FLAGS "$@" || exit 1
eval set -- "$FLAGS_ARGV"

# Globals
# ----------------------------------------------------------------------------
set -e

# Flashrom commands with device overrides
FLASHROM_BIOS="flashrom -p host"
FLASHROM_EC="flashrom -p ec"

# A log file to keep the output results of executed command
EXEC_LOG="$(make_temp_file)"

# Temporary Work directory
WORK_DIR="$(make_temp_dir)"
OLD_BIOS="$WORK_DIR/old_bios.bin"
NEW_BIOS="$WORK_DIR/new_bios.bin"

# Functions
# ----------------------------------------------------------------------------

# Error message for write protect disable failure, with reminder
wp_error() {
  local which_rom=$1
  shift
  echo "ERROR: Unable to disable $which_rom write protect: $*" 1>&2
  echo "Is hardware write protect still enabled?" 1>&2
  exit 1
}

# Disable write protect for an EEPROM
disable_wp() {
  local which_rom=$1  # EC or BIOS
  shift
  local flash_rom="$*"  # Flashrom command to use

  debug_msg "Disabling $which_rom write protect"
  $NOTHING ${flash_rom} --wp-disable || wp_error "$which_rom" "--wp-disable"
  $NOTHING ${flash_rom} --wp-range 0 0 || wp_error "$which_rom" "--wp-range"

  # WP status bits should report WP: status: 0x00
  local wp_status="$(${flash_rom} --wp-status | grep "WP: status:")"
  if [ "$wp_status" != "WP: status: 0x00" ]; then
    if [ "$FLAGS_nothing" = $FLAGS_FALSE ]; then
      wp_error "$which_rom" "$wp_status"
    fi
  fi
}

# Back up current firmware and partition table
make_backups() {
  debug_msg "Backing up current firmware to $FLAGS_backup_dir"
  mkdir -p "$FLAGS_backup_dir"
  cp "$OLD_BIOS" "$FLAGS_backup_dir/$FLAGS_asset_tag.bios.bin"
  ${FLASHROM_EC} -r "$FLAGS_backup_dir/$FLAGS_asset_tag.ec.bin"

  # Copy the VPD info from RAM, since we can't extract it as text
  # from the BIOS binary.  Failure of this is only a warning, since
  # the information is still in the old BIOS.
  mosys vpd print all > "$FLAGS_backup_dir/$FLAGS_asset_tag.vpd.txt" ||
    echo "WARNING: unable to save VPD as text."

  # Copy the first part of the drive, so we can recreate the partition
  # table.
  local gpt_backup="$FLAGS_backup_dir/$FLAGS_asset_tag.gpt.bin"
  debug_msg "Backing up current GPT table."
  dd if="$FLAGS_ssd" of="$gpt_backup" bs=512 count=34

  # Add a script to restore the BIOS and GPT
  local restore_script="$FLAGS_backup_dir/$FLAGS_asset_tag.restore.sh"
  cat >"$restore_script" <<EOF
#!/bin/sh
echo "Restoring BIOS"
${FLASHROM_BIOS} -w "$FLAGS_asset_tag.bios.bin"
echo "Restoring EC"
${FLASHROM_EC} -w "$FLAGS_asset_tag.ec.bin"
echo "Restoring GPT"
dd of="$FLAGS_ssd" if="$FLAGS_asset_tag.gpt.bin" conv=notrunc
EOF
  echo "To restore original BIOS and SSD:"
  echo "  cd $FLAGS_backup_dir && sh $FLAGS_asset_tag.restore.sh"
}

# Main
# ----------------------------------------------------------------------------

main() {
  # Make sure the files we were passed exist
  [ -n "$FLAGS_bios" ] || err_die "Please specify a BIOS file (-b bios.bin)"
  [ -n "$FLAGS_ec" ] || err_die "Please specify an EC updater (-e updater.sh)"
  ensure_files_exist "$FLAGS_bios" "$FLAGS_ec" || exit 1

  # If --nothing was specified, keep flashrom from writing
  if [ "$FLAGS_nothing" = $FLAGS_TRUE ]; then
    NOTHING="echo Not executing: "
  fi

  # Stop update engine before calling flashrom.  Multiple copies of flashrom
  # interfere with each other.
  debug_msg "Stopping update engine"
  initctl stop update-engine

  # Read the current firmware
  debug_msg "Reading BIOS from EEPROM"
  ${FLASHROM_BIOS} -r "$OLD_BIOS"

  # Copy current info to the backup dir, if specified
  if [ -n "$FLAGS_backup_dir" ]; then
    make_backups
  fi

  # Find the RO VPD area in the current firmware
  local t="$(mosys -k eeprom map "$OLD_BIOS" | grep 'RO VPD')"
  local vpd_offset="$(echo $t | sed 's/.*area_offset="\([^"]*\)" .*/\1/' )"
  local vpd_size="$(echo $t | sed 's/.*area_size="\([^"]*\)" .*/\1/' )"
  debug_msg "Found VPD at offset $vpd_offset size $vpd_size"
  # Convert offset and size to decimal, since dd doesn't grok hex
  vpd_offset="$(printf "%d" $vpd_offset)"
  vpd_size="$(printf "%d" $vpd_size)"
  debug_msg "In decimal, VPD is at offset $vpd_offset size $vpd_size"

  # Copy the RO VPD from the old firmware to the new firmware
  debug_msg "Copying VPD from old firmware to new firmware"
  cp "$FLAGS_bios" "$NEW_BIOS"
  dd bs=1 seek=$vpd_offset skip=$vpd_offset count=$vpd_size conv=notrunc \
    if="$OLD_BIOS" of="$NEW_BIOS" || err_die "Unable to copy RO VPD"

  # Disable write protect
  disable_wp "EC" ${FLASHROM_EC}
  disable_wp "BIOS" ${FLASHROM_BIOS}

  # Write new firmware
  debug_msg "Writing EC"
  # TODO: if EC file ends in .bin, use flashrom to write it directly?
  $NOTHING sh "$FLAGS_ec" --factory || err_die "Unable to write EC"
  debug_msg "Writing BIOS"
  $NOTHING ${FLASHROM_BIOS} -w "$NEW_BIOS" || err_die "Unable to write BIOS"

  # Wipe SSD
  if [ "$FLAGS_wipe_ssd" = $FLAGS_TRUE ]; then
    debug_msg "Wiping SSD"
    $NOTHING cgpt create -z "$FLAGS_ssd" || err_die "Unable to wipe SSD"
  fi

  # Leave the update engine stopped.  We've mucked with the firmware
  # and SSD contents, so we shouldn't be attempting an autoupdate this
  # boot anyway.
}

main
