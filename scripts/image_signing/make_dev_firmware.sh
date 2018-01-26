#!/bin/sh
#
# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
#
# This script can change key (usually developer keys) in a firmware binary
# image or system live firmware (EEPROM), and assign proper HWID, FLAGS as well.

SCRIPT_BASE="$(dirname "$0")"
. "$SCRIPT_BASE/common_minimal.sh"
load_shflags || exit 1

# Constants used by DEFINE_*
VBOOT_BASE='/usr/share/vboot'
DEFAULT_KEYS_FOLDER="$VBOOT_BASE/devkeys"
DEFAULT_BACKUP_FOLDER='/mnt/stateful_partition/backups'

# DEFINE_string name default_value description flag
DEFINE_string from "" "Path of input BIOS file (empty for system live BIOS)" "f"
DEFINE_string to "" "Path of output BIOS file (empty for system live BIOS)" "t"
DEFINE_string ec_from "" "Path of input EC file (empty for system live EC)" "e"
DEFINE_string ec_to "" "Path of output EC file (empty for system live EC)" "o"
DEFINE_string keys "$DEFAULT_KEYS_FOLDER" "Path to folder of dev keys" "k"
DEFINE_string preamble_flags "" "Override preamble flags value. Known values:
                        0: None. (Using RW to boot in normal. aka, two-stop)
                        1: VB_FIRMWARE_PREAMBLE_USE_RO_NORMAL (one-stop)" "p"
DEFINE_boolean mod_hwid \
  $FLAGS_TRUE "Modify HWID to indicate this is a modified firmware" ""
DEFINE_boolean mod_gbb_flags \
  $FLAGS_TRUE "Modify GBB flags to enable developer friendly features" ""
DEFINE_boolean change_ec \
  $FLAGS_FALSE "Change the key in the EC binary if EC uses EFS boot" ""
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
IMAGE_BIOS="$(make_temp_file)"
IMAGE_BIOS="$(readlink -f "${IMAGE_BIOS}")"
if [ "${FLAGS_change_ec}" = "${FLAGS_TRUE}" ]; then
  IMAGE_EC="$(make_temp_file)"
  IMAGE_EC="$(readlink -f "${IMAGE_EC}")"
fi

# a log file to keep the output results of executed command
EXEC_LOG="$(make_temp_file)"

# Functions
# ----------------------------------------------------------------------------

flashrom_bios() {
  if is_debug_mode; then
    flashrom -V -p host "$@"
  else
    flashrom -p host "$@"
  fi
}

flashrom_ec() {
  if is_debug_mode; then
    flashrom -V -p ec "$@"
  else
    flashrom -p ec "$@"
  fi
}

# Execute the given command and log its output to the file ${EXEC_LOG}.
# If is_debug_mode, also print the output directly.
execute() {
  if is_debug_mode; then
    "$@" 2>&1 | tee "${EXEC_LOG}"
  else
    "$@" >"${EXEC_LOG}" 2>&1
  fi
}

# Disables write protection status registers
disable_write_protection() {
  # No need to change WP status in file mode
  if [ -n "$FLAGS_to" ]; then
    return $FLAGS_TRUE
  fi

  # --wp-disable command may return success even if WP is still enabled,
  # so we should use --wp-status to verify the results.
  echo "Disabling system software write protection status..."
  (flashrom_bios --wp-disable && flashrom_bios --wp-status) 2>&1 |
    tee "$EXEC_LOG" |
    grep -q '^WP: .* is disabled\.$'
}

# Reads ${IMAGE_BIOS} from ${FLAGS_from} and ${IMAGE_EC} from ${FLAGS_ec_from}
read_image() {
  if [ -z "$FLAGS_from" ]; then
    echo "Reading system live BIOS firmware..."
    execute flashrom_bios -r "${IMAGE_BIOS}"
  else
    debug_msg "reading from file: ${FLAGS_from}"
    cp -f "${FLAGS_from}" "${IMAGE_BIOS}"
  fi
  if [ "${FLAGS_change_ec}" = "${FLAGS_TRUE}" ]; then
    if [ -z "${FLAGS_ec_from}" ]; then
      echo "Reading system live EC firmware..."
      execute flashrom_ec -r "${IMAGE_EC}"
    else
      debug_msg "reading from file: ${FLAGS_ec_from}"
      cp -f "${FLAGS_ec_from}" "${IMAGE_EC}"
    fi
  fi
}

# Writes ${IMAGE_BIOS} to ${FLAGS_to} and ${IMAGE_EC} to ${FLAGS_ec_to}
write_image() {
  if [ -z "${FLAGS_to}" ]; then
    echo "Writing system live BIOS firmware..."
    # TODO(hungte) we can enable partial write to make this faster
    execute flashrom_bios -w "${IMAGE_BIOS}"
  else
    debug_msg "writing to file: ${FLAGS_to}"
    cp -f "${IMAGE_BIOS}" "${FLAGS_to}"
    chmod a+r "${FLAGS_to}"
  fi
  if [ "${FLAGS_change_ec}" = "${FLAGS_TRUE}" ]; then
    if [ -z "${FLAGS_ec_to}" ]; then
      echo "Writing system live EC firmware..."
      # TODO(hungte) we can enable partial write to make this faster
      execute flashrom_ec -w "${IMAGE_EC}"
    else
      debug_msg "writing to file: ${FLAGS_ec_to}"
      cp -f "${IMAGE_EC}" "${FLAGS_ec_to}"
      chmod a+r "${FLAGS_ec_to}"
    fi
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

# Main
# ----------------------------------------------------------------------------
main() {
  # Check parameters
  local root_pubkey="${FLAGS_keys}/root_key.vbpubk"
  local recovery_pubkey="${FLAGS_keys}/recovery_key.vbpubk"
  local firmware_keyblock="${FLAGS_keys}/firmware.keyblock"
  local firmware_prvkey="${FLAGS_keys}/firmware_data_key.vbprivk"
  local dev_firmware_keyblock="${FLAGS_keys}/dev_firmware.keyblock"
  local dev_firmware_prvkey="${FLAGS_keys}/dev_firmware_data_key.vbprivk"
  local kernel_sub_pubkey="${FLAGS_keys}/kernel_subkey.vbpubk"
  local ec_efs_pubkey="${FLAGS_keys}/key_ec_efs.vbpubk2"
  local ec_efs_prvkey="${FLAGS_keys}/key_ec_efs.vbprik2"
  local is_from_live=0
  local backup_bios_image=''
  local backup_ec_image=''

  debug_msg "Prerequisite check"
  ensure_files_exist \
    "${root_pubkey}" \
    "${recovery_pubkey}" \
    "${firmware_keyblock}" \
    "${firmware_prvkey}" \
    "${kernel_sub_pubkey}" \
    "${ec_efs_pubkey}" \
    "${ec_efs_prvkey}" ||
    exit 1

  if [ -z "${FLAGS_from}" ]; then
    is_from_live=1
  else
    ensure_files_exist "${FLAGS_from}" || exit 1
  fi

  if [ -z "${FLAGS_ec_from}" ]; then
    is_from_live=1
  else
    ensure_files_exist "${FLAGS_ec_from}" || exit 1
  fi

  debug_msg "Checking software write protection status"
  disable_write_protection ||
    if is_debug_mode; then
      die "Failed to disable WP. Diagnose Message: $(cat "${EXEC_LOG}")"
    else
      die "Write protection is still enabled. " \
          "Please verify that hardware write protection is disabled."
    fi

  debug_msg "Pulling image"
  (read_image &&
      [ -s "${IMAGE_BIOS}" ] &&
      [ "${FLAGS_change_ec}" = "${FLAGS_FALSE}" -o -s "${IMAGE_EC}" ]) ||
    die "Failed to read image. Error message: $(cat "${EXEC_LOG}")"


  debug_msg "Prepare to backup the file"
  if [ -n "${is_from_live}" -o ${FLAGS_force_backup} = ${FLAGS_TRUE} ]; then
    backup_bios_image="$(make_temp_file)"
    debug_msg "Creating BIOS backup file to ${backup_bios_image}..."
    cp -f "${IMAGE_BIOS}" "${backup_bios_image}"

    if [ "${FLAGS_change_ec}" = "${FLAGS_TRUE}" ]; then
      backup_ec_image="$(make_temp_file)"
      debug_msg "Creating EC backup file to ${backup_ec_image}..."
      cp -f "${IMAGE_EC}" "${backup_ec_image}"
    fi
  fi

  local expanded_firmware_dir="$(make_temp_dir)"
  if [ "${FLAGS_change_ec}" = "${FLAGS_TRUE}" ]; then
    if is_ec_rw_signed "${IMAGE_EC}"; then
      # TODO(waihong): These are duplicated from sign_official_build.sh. We need
      # to move them to a single place for share.
      debug_msg "Resign EC firmware with new EC EFS key"
      local rw_bin="EC_RW.bin"
      local rw_hash="EC_RW.hash"
      # futility writes byproduct files to CWD, so we cd to temp dir.
      local old_cwd=$(pwd)
      cd "${expanded_firmware_dir}"

      ${FUTILITY} sign --type rwsig --prikey "${ec_efs_prvkey}" "${IMAGE_EC}" ||
        die "Failed to sign EC image"
      # Above command produces EC_RW.bin. Compute its hash.
      openssl dgst -sha256 -binary "${rw_bin}" > "${rw_hash}"

      debug_msg "Store ecrw and its hash to BIOS firmware"
      store_file_in_cbfs "${IMAGE_BIOS}" "${rw_bin}" "ecrw" ||
        die "Failed to store ecrw in BIOS image"
      store_file_in_cbfs "${IMAGE_BIOS}" "${rw_hash}" "ecrw.hash" ||
        die "Failed to store ecrw.hash in BIOS image"

      cd "${old_cwd}"
      # Continuous the code below to resign the BIOS image.
    else
      echo "EC image is not signed. Skip changing its key."
    fi
  fi

  debug_msg "Detecting developer firmware keyblock"
  local use_devfw_keyblock="$FLAGS_FALSE"
  (cd "${expanded_firmware_dir}"; dump_fmap -x "${IMAGE_BIOS}" \
    >/dev/null 2>&1) || die "Failed to extract firmware image."
  if [ -f "$expanded_firmware_dir/VBLOCK_A" ]; then
    local has_dev=$FLAGS_TRUE has_norm=$FLAGS_TRUE
    # In output of vbutil_keyblock, "!DEV" means "bootable on normal mode" and
    # "DEV" means "bootable on developer mode". Here we try to match the pattern
    # in output of vbutil_block, and disable the flags (has_dev, has_norm) if
    # the pattern was not found.
    vbutil_keyblock --unpack "$expanded_firmware_dir/VBLOCK_A" |
      grep -qw '!DEV' || has_norm=$FLAGS_FALSE
    vbutil_keyblock --unpack "$expanded_firmware_dir/VBLOCK_A" |
      grep -qw '[^!]DEV' || has_dev=$FLAGS_FALSE
    if [ "$has_norm" = "$FLAGS_FALSE" -a "$has_dev" = "$FLAGS_TRUE" ]; then
      use_devfw_keyblock=$FLAGS_TRUE
    fi
  fi

  if [ "$use_devfw_keyblock" = "$FLAGS_TRUE" ]; then
    echo "Using keyblocks (developer, normal)..."
  else
    echo "Using keyblocks (normal, normal)..."
    dev_firmware_prvkey="$firmware_prvkey"
    dev_firmware_keyblock="$firmware_keyblock"
  fi

  debug_msg "Extract firmware version and data key version"
  ${FUTILITY} gbb -g --rootkey="${expanded_firmware_dir}/rootkey" \
    "${IMAGE_BIOS}" >/dev/null 2>&1

  local data_key_version firmware_version
  # When we are going to flash directly from or to system, the versions stored
  # in TPM can be found by crossystem; otherwise we'll need to guess from source
  # firmware (FLAGS_from).
  if [ -z "$FLAGS_to" -o -z "$FLAGS_from" ]; then
    debug_msg "Reading TPM version from crossystem tpm_fwver."
    data_key_version="$(( $(crossystem tpm_fwver) >> 16 ))"
    firmware_version="$(( $(crossystem tpm_fwver) & 0xFFFF ))"
  else
    # TODO(hungte) On Vboot2, A/B slot may contain different firmware so we may
    # need to check both and decide from largest number.
    debug_msg "Guessing TPM version from original firmware."
    local fw_info="$(vbutil_firmware \
                     --verify "${expanded_firmware_dir}/VBLOCK_A" \
                     --signpubkey "${expanded_firmware_dir}/rootkey" \
                     --fv "${expanded_firmware_dir}/FW_MAIN_A" 2>/dev/null)" ||
        die "Failed to verify firmware slot A."
    data_key_version="$(
      echo "$fw_info" | sed -n '/^ *Data key version:/s/.*:[ \t]*//p')"
    firmware_version="$(
      echo "$fw_info" | sed -n '/^ *Firmware version:/s/.*:[ \t]*//p')"
  fi

  local new_data_key_version="$(
    vbutil_keyblock --unpack "$firmware_keyblock" |
    sed -n '/^ *Data key version:/s/.*:[ \t]*//p')"

  # TODO(hungte) Change key block by data_key_version.
  if [ "$data_key_version" -gt "$new_data_key_version" ]; then
    echo "$(tput bold)$(tput setaf 1)
    Warning: firmware data key version <$new_data_key_version> in your new keys
    [$FLAGS_keys] is smaller than original firmware <$data_key_version> and
    will boot into only recovery mode due to TPM anti-rollback detection.

    After reboot with dev recovery key, you will need to reset TPM by booting a
    test or dev image in recovery mode (NOT Ctrl-U), switch to VT2 and run
    command <chromoes-tpm-recovery>; or use a factory install shim image
    (build_image factory_install).
    $(tput sgr 0)" >&2
  fi

  echo "Signing with Data Key Version: $data_key_version, " \
       "Firmware Version: $firmware_version"

  echo "Preparing new firmware image..."

  debug_msg "Resign the firmware code (A/B) with new keys"
  # Note resign_firmwarefd.sh needs the original rootkey to determine firmware
  # body size, so we must resign image before changing GBB rootkey.

  local unsigned_image="$(make_temp_file)"
  local optional_opts=""
  if [ -n "$FLAGS_preamble_flags" ]; then
    debug_msg "Setting FLAGS=$FLAGS_preamble_flags"
    optional_opts="$FLAGS_preamble_flags"
  fi
  cp -f "${IMAGE_BIOS}" "$unsigned_image"
  execute "$SCRIPT_BASE/resign_firmwarefd.sh" \
    "${unsigned_image}" \
    "${IMAGE_BIOS}" \
    "${firmware_prvkey}" \
    "${firmware_keyblock}" \
    "${dev_firmware_prvkey}" \
    "${dev_firmware_keyblock}" \
    "${kernel_sub_pubkey}" \
    "${firmware_version}" \
    ${optional_opts} ||
    die "Failed to re-sign firmware. (message: $(cat "${EXEC_LOG}"))"

  debug_msg "Extract current HWID"
  local old_hwid
  old_hwid="$(${FUTILITY} gbb --get --hwid "${IMAGE_BIOS}" 2>"${EXEC_LOG}" |
              sed -rne 's/^hardware_id: (.*)$/\1/p')"

  debug_msg "Decide new HWID"
  [ -z "$old_hwid" ] &&
    die "Cannot find current HWID. (message: $(cat "${EXEC_LOG}"))"
  local new_hwid="$old_hwid"
  if [ "$FLAGS_mod_hwid" = "$FLAGS_TRUE" ]; then
    new_hwid="$(echo_dev_hwid "$old_hwid")"
  fi

  local old_gbb_flags
  old_gbb_flags="$(${FUTILITY} gbb --get --flags "${IMAGE_BIOS}" \
    2>"${EXEC_LOG}" | sed -rne 's/^flags: (.*)$/\1/p')"
  debug_msg "Decide new GBB flags from: $old_gbb_flags"
  [ -z "$old_gbb_flags" ] &&
    die "Cannot find GBB flags. (message: $(cat "${EXEC_LOG}"))"
  # 0x30: GBB_FLAG_FORCE_DEV_BOOT_USB | GBB_FLAG_DISABLE_FW_ROLLBACK_CHECK
  local new_gbb_flags="$((old_gbb_flags | 0x30))"

  debug_msg "Replace GBB parts (futility gbb allows changing on-the-fly)"
  ${FUTILITY} gbb --set \
    --hwid="${new_hwid}" \
    --rootkey="${root_pubkey}" \
    --recoverykey="${recovery_pubkey}" \
    "${IMAGE_BIOS}" >"${EXEC_LOG}" 2>&1 ||
    die "Failed to change GBB Data. (message: $(cat "${EXEC_LOG}"))"

  # Old firmware does not support GBB flags, so let's make it an exception.
  if [ "${FLAGS_mod_gbb_flags}" = "${FLAGS_TRUE}" ]; then
    debug_msg "Changing GBB flags from ${old_gbb_flags} to ${new_gbb_flags}"
    ${FUTILITY} gbb --set \
      --flags="${new_gbb_flags}" \
      "${IMAGE_BIOS}" >"${EXEC_LOG}" 2>&1 ||
      echo "Warning: Can't set GBB flags ${old_gbb_flags} -> ${new_gbb_flags}."
  fi

  # TODO(hungte) compare if the image really needs to be changed.

  debug_msg "Check if we need to make backup file(s)"
  if [ -n "${backup_bios_image}" ]; then
    local backup_hwid_name="$(echo "${old_hwid}" | sed 's/ /_/g')"
    local backup_date_time="$(date +'%Y%m%d_%H%M%S')"
    local backup_bios_name="bios_${backup_hwid_name}_${backup_date_time}.fd"
    local backup_bios_path="${FLAGS_backup_dir}/${backup_bios_name}"
    local backup_ec_name="ec_${backup_hwid_name}_${backup_date_time}.fd"
    local backup_ec_path=''
    if mkdir -p "${FLAGS_backup_dir}" &&
       cp -f "${backup_bios_image}" "${backup_bios_path}"; then
      if [ -n "${backup_ec_image}" ]; then
        backup_ec_path="${FLAGS_backup_dir}/${backup_ec_name}"
        cp -f "${backup_ec_image}" "${backup_ec_path}"
      fi
    elif cp -f "${backup_bios_image}" "/tmp/${backup_bios_name}"; then
      backup_bios_path="/tmp/${backup_bios_name}"
      if [ -n "${backup_ec_image}" ]; then
        cp -f "${backup_ec_image}" "/tmp/${backup_ec_name}"
        backup_ec_path="/tmp/${backup_ec_name}"
      fi
    else
      backup_bios_path=''
    fi
    if [ -n "${backup_bios_path}" ]; then
      # TODO(hungte) maybe we can wrap the flashrom by 'make_dev_firmware.sh -r'
      # so that the only command to remember would be make_dev_firmware.sh.
      echo "
        Backup of current firmware image is stored in:
          ${backup_bios_path}
          ${backup_ec_path}
        Please copy the backup file to a safe place ASAP.

        To stop using devkeys and restore original BIOS, execute command:
          flashrom -p bios -w [PATH_TO_BACKUP_BIOS]
        Ex: flashrom -p bios -w ${backup_bios_path}"
      if [ -n "${backup_ec_image}" ]; then
        echo "
        To stop using devkeys and restore original EC, execute command:
          flashrom -p ec -w [PATH_TO_BACKUP_EC]
        Ex: flashrom -p ec -w ${backup_ec_path}"
      fi
      echo ""
    else
      echo "WARNING: Can't create file in ${FLAGS_backup_dir}. Ignore backups."
    fi
  fi

  # TODO(hungte) use vbutil_firmware to check if the new firmware is valid.
  # Or, do verification in resign_firmwarefd.sh and trust it.

  debug_msg "Write the image"
  write_image ||
    die "Failed to write image. Error message: $(cat "${EXEC_LOG}")"

  debug_msg "Complete."
  if [ -z "$FLAGS_to" ]; then
    echo "Successfully changed firmware to Developer Keys. New HWID: $new_hwid"
  else
    echo "Firmware '$FLAGS_to' now uses Developer Keys. New HWID: $new_hwid"
  fi
}

main
