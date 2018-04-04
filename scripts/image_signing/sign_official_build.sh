#!/bin/bash

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Sign the final build image using the "official" keys.
#
# Prerequisite tools needed in the system path:
#
#  futility (from src/platform/vboot_reference)
#  vbutil_kernel (from src/platform/vboot_reference)
#  vbutil_key (from src/platform/vboot_reference)
#  cgpt (from src/platform/vboot_reference)
#  dump_kernel_config (from src/platform/vboot_reference)
#  verity (from src/platform/verity)
#  load_kernel_test (from src/platform/vboot_reference)
#  dumpe2fs
#  sha1sum
#  cbfstool (from src/third_party/coreboot)

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

# Print usage string
usage() {
  cat <<EOF
Usage: $PROG <type> input_image /path/to/keys/dir [output_image] [version_file]
where <type> is one of:
             ssd  (sign an SSD image)
             base (sign a base image, similar to an SSD image)
             recovery (sign a USB recovery image)
             factory (sign a factory install image)
             install (old alias to "factory")
             update_payload (sign a delta update hash)
             kernel (sign a kernel image)
             recovery_kernel (sign a recovery_kernel image)
             firmware (sign a firmware image)
             usb  (sign an image to boot directly from USB)
             verify (verify an image including rootfs hashes)
             nv_lp0_firmware (sign nvidia lp0 firmware)
             accessory_usbpd (sign USB-PD accessory firmware)
             accessory_rwsig (sign accessory RW firmware)
             oci-container (sign an OCI container)

output_image: File name of the signed output image
version_file: File name of where to read the kernel and firmware versions.

If you are signing an image, you must specify an [output_image] and
optionally, a [version_file].

EOF
  if [[ $# -gt 0 ]]; then
    error "$*"
    exit 1
  fi
  exit 0
}

# Verify we have as many arguments as we expect, else show usage & quit.
# Usage:
#  check_argc <number args> <exact number>
#  check_argc <number args> <lower bound> <upper bound>
check_argc() {
  case $# in
  2)
    if [[ $1 -ne $2 ]]; then
      usage "command takes exactly $2 args"
    fi
    ;;
  3)
    if [[ $1 -lt $2 || $1 -gt $3 ]]; then
      usage "command takes $2 to $3 args"
    fi
    ;;
  *)
    die "check_argc: incorrect number of arguments"
  esac
}

# Abort on errors.
set -e

# Add to the path since some tools reside here and may not be in the non-root
# system path.
PATH=$PATH:/usr/sbin:/sbin

# Make sure the tools we need are available.
for prereqs in ${FUTILITY} vbutil_kernel cgpt dump_kernel_config verity \
               cbfstool load_kernel_test dumpe2fs sha1sum e2fsck; do
  type -P "${prereqs}" &>/dev/null || \
    die "${prereqs} tool not found."
done

TYPE=$1
INPUT_IMAGE=$2
KEY_DIR=$3
OUTPUT_IMAGE=$4
VERSION_FILE=$5

FIRMWARE_VERSION=1
KERNEL_VERSION=1

# TODO(gauravsh): These are duplicated from chromeos-setimage. We need
# to move all signing and rootfs code to one single place where it can be
# reused. crosbug.com/19543

# get_verity_arg <commandline> <key> -> <value>
get_verity_arg() {
  echo "$1" | sed -n "s/.*\b$2=\([^ \"]*\).*/\1/p"
}

is_old_verity_argv() {
  local depth=$(echo "$1" | cut -f7 -d' ')
  if [ "$depth" = "0" ]; then
    return 0
  fi
  return 1
}

# Get the dmparams parameters from a kernel config.
get_dmparams_from_config() {
  local kernel_config=$1
  echo ${kernel_config} | sed -nre 's/.*dm="([^"]*)".*/\1/p'
}
# Get the verity root digest hash from a kernel config command line.
get_hash_from_config() {
  local kernel_config=$1
  local dm_config=$(get_dmparams_from_config "${kernel_config}")
  local vroot_dev=$(get_dm_slave "${dm_config}" vroot)
  if is_old_verity_argv "${vroot_dev}"; then
    echo ${vroot_dev} | cut -f9 -d ' '
  else
    echo $(get_verity_arg "${vroot_dev}" root_hexdigest)
  fi
}

# Get the slave device and its args
# get_dm_ags $dm_config [vboot|vroot]
# Assumes we have only one slave device per device
get_dm_slave() {
  local dm=$1
  local device=$2
  echo $(echo "${dm}" | sed -nre "s/.*${device}[^,]*,([^,]*).*/\1/p")
}

# Set the slave device and its args for a device
# get_dm_ags $dm_config [vboot|vroot] args
# Assumes we have only one slave device per device
set_dm_slave() {
  local dm=$1
  local device=$2
  local slave=$3
  echo $(echo "${dm}" |
    sed -nre "s#(.*${device}[^,]*,)([^,]*)(.*)#\1${slave}\3#p")
}

CALCULATED_KERNEL_CONFIG=
CALCULATED_DM_ARGS=
# Calculate rootfs hash of an image
# Args: ROOTFS_IMAGE KERNEL_CONFIG HASH_IMAGE
#
# rootfs calculation parameters are grabbed from KERNEL_CONFIG
#
# Updated dm-verity arguments (to be replaced in kernel config command line)
# with the new hash is stored in $CALCULATED_DM_ARGS and the new hash image is
# written to the file HASH_IMAGE.
calculate_rootfs_hash() {
  local rootfs_image=$1
  local kernel_config=$2
  local hash_image=$3
  local dm_config=$(get_dmparams_from_config "${kernel_config}")

  if [ -z "${dm_config}" ]; then
    warn "Couldn't grab dm_config. Aborting rootfs hash calculation."
    return 1
  fi
  local vroot_dev=$(get_dm_slave "${dm_config}" vroot)

  local rootfs_sectors
  local verity_depth
  local verity_algorithm
  local root_dev
  local hash_dev
  local verity_bin="verity"
  if is_old_verity_argv "${vroot_dev}"; then
    # dm="0 2097152 verity ROOT_DEV HASH_DEV 2097152 1 \
    # sha1 63b7ad16cb9db4b70b28593f825aa6b7825fdcf2"
    rootfs_sectors=$(echo ${vroot_dev} | cut -f2 -d' ')
    verity_depth=$(echo ${vroot_dev} | cut -f7 -d' ')
    verity_algorithm=$(echo ${vroot_dev} | cut -f8 -d' ')
    root_dev=$(echo ${vroot_dev} | cut -f4 -d ' ')
    hash_dev=$(echo ${vroot_dev} | cut -f5 -d ' ')
    # Hack around the fact that the signer needs to use the old version of
    # verity to generate legacy verity kernel parameters. If we find it,
    # we use it.
    type -P "verity-old" &>/dev/null && verity_bin="verity-old"
  else
    # Key-value parameters.
    rootfs_sectors=$(get_verity_arg "${vroot_dev}" hashstart)
    verity_depth=0
    verity_algorithm=$(get_verity_arg "${vroot_dev}" alg)
    root_dev=$(get_verity_arg "${vroot_dev}" payload)
    hash_dev=$(get_verity_arg "${vroot_dev}" hashtree)
    salt=$(get_verity_arg "${vroot_dev}" salt)
  fi

  local salt_arg
  if [ -n "$salt" ]; then
    salt_arg="salt=$salt"
  fi

  # Run the verity tool on the rootfs partition.
  local slave=$(sudo ${verity_bin} mode=create \
    alg=${verity_algorithm} \
    payload="${rootfs_image}" \
    payload_blocks=$((rootfs_sectors / 8)) \
    hashtree="${hash_image}" ${salt_arg})
  # Reconstruct new kernel config command line and replace placeholders.
  slave="$(echo "${slave}" |
    sed -s "s|ROOT_DEV|${root_dev}|g;s|HASH_DEV|${hash_dev}|")"
  CALCULATED_DM_ARGS="$(set_dm_slave "${dm_config}" vroot "${slave}")"
  CALCULATED_KERNEL_CONFIG="$(echo "${kernel_config}" |
    sed -e 's#\(.*dm="\)\([^"]*\)\(".*\)'"#\1${CALCULATED_DM_ARGS}\3#g")"
}

# Re-calculate rootfs hash, update rootfs and kernel command line(s).
# Args: LOOPDEV KERNEL KERN_A_KEYBLOCK KERN_A_PRIVKEY KERN_B_KEYBLOCK \
#       KERN_B_PRIVKEY
#
# The rootfs is hashed by tool 'verity', and the hash data is stored after the
# rootfs. A hash of those hash data (also known as final verity hash) may be
# contained in kernel 2 or kernel 4 command line.
#
# This function reads dm-verity configuration from KERNEL, rebuilds the rootfs
# hash, and then resigns kernel A & B by their keyblock and private key files.
update_rootfs_hash() {
  local loopdev="$1"  # Input image.
  local loop_kern="$2"  # Kernel that contains verity args.
  local kern_a_keyblock="$3"  # Keyblock file for kernel A.
  local kern_a_privkey="$4"  # Private key file for kernel A.
  local kern_b_keyblock="$5"  # Keyblock file for kernel B.
  local kern_b_privkey="$6"  # Private key file for kernel A.
  local loop_rootfs="${loopdev}p3"

  # Note even though there are two kernels, there is one place (after rootfs)
  # for hash data, so we must assume both kernel use same hash algorithm (i.e.,
  # DM config).
  info "Updating rootfs hash and updating config for Kernel partitions"

  # If we can't find dm parameters in the kernel config, bail out now.
  local kernel_config=$(sudo dump_kernel_config "${loop_kern}")
  local dm_config=$(get_dmparams_from_config "${kernel_config}")
  if [ -z "${dm_config}" ]; then
    error "Couldn't grab dm_config from kernel ${loop_kern}"
    error " (config: ${kernel_config})"
    return 1
  fi

  # check and clear need_to_resign tag
  local rootfs_dir=$(make_temp_dir)
  sudo mount -o ro "${loop_rootfs}" "${rootfs_dir}"
  if has_needs_to_be_resigned_tag "${rootfs_dir}"; then
    # remount as RW
    sudo mount -o remount,rw "${rootfs_dir}"
    sudo rm -f "${rootfs_dir}/${TAG_NEEDS_TO_BE_SIGNED}"
  fi
  sudo umount "${rootfs_dir}"

  local hash_image=$(make_temp_file)

  # Disable rw mount support prior to hashing.
  disable_rw_mount "${loop_rootfs}"

  if ! calculate_rootfs_hash "${loop_rootfs}"  "${kernel_config}" \
    "${hash_image}"; then
    error "calculate_rootfs_hash failed!"
    error "Aborting rootfs hash update!"
    return 1
  fi

  local rootfs_blocks=$(sudo dumpe2fs "${loop_rootfs}" 2> /dev/null |
    grep "Block count" |
    tr -d ' ' |
    cut -f2 -d:)
  local rootfs_sectors=$((rootfs_blocks * 8))

  # Overwrite the appended hashes in the rootfs
  sudo dd if="${hash_image}" of="${loop_rootfs}" bs=512 \
    seek=${rootfs_sectors} conv=notrunc 2>/dev/null

  # Update kernel command lines
  local dm_args="${CALCULATED_DM_ARGS}"
  local temp_config=$(make_temp_file)
  local kernelpart=
  local keyblock=
  local priv_key=
  local new_kernel_config=

  for kernelpart in 2 4; do
    loop_kern="${loopdev}p${kernelpart}"
    if ! new_kernel_config="$(
         sudo dump_kernel_config "${loop_kern}" 2>/dev/null)" &&
       [[ "${kernelpart}" == 4 ]]; then
      # Legacy images don't have partition 4.
      info "Skipping empty kernel partition 4 (legacy images)."
      continue
    fi
    new_kernel_config="$(echo "${new_kernel_config}" |
      sed -e 's#\(.*dm="\)\([^"]*\)\(".*\)'"#\1${dm_args}\3#g")"
    info "New config for kernel partition ${kernelpart} is:"
    echo "${new_kernel_config}" | tee "${temp_config}"
    # Re-calculate kernel partition signature and command line.
    if [[ "$kernelpart" == 2 ]]; then
      keyblock="${kern_a_keyblock}"
      priv_key="${kern_a_privkey}"
    else
      keyblock="${kern_b_keyblock}"
      priv_key="${kern_b_privkey}"
    fi
    sudo vbutil_kernel --repack "${loop_kern}" \
      --keyblock ${keyblock} \
      --signprivate ${priv_key} \
      --version "${KERNEL_VERSION}" \
      --oldblob "${loop_kern}" \
      --config ${temp_config}
  done
}

# Update the SSD install-able vblock file on stateful partition.
# ARGS: Loopdev
# This is deprecated because all new images should have a SSD boot-able kernel
# in partition 4. However, the signer needs to be able to sign new & old images
# (crbug.com/449450#c13) so we will probably never remove this.
update_stateful_partition_vblock() {
  local loopdev="$1"
  local temp_out_vb="$(make_temp_file)"

  local loop_kern="${loopdev}p4"
  if [[ -z "$(sudo dump_kernel_config "${loop_kern}" 2>/dev/null)" ]]; then
    info "Building vmlinuz_hd.vblock from legacy image partition 2."
    loop_kern="${loopdev}p2"
  fi

  # vblock should always use kernel keyblock.
  sudo vbutil_kernel --repack "${temp_out_vb}" \
    --keyblock "${KEY_DIR}/kernel.keyblock" \
    --signprivate "${KEY_DIR}/kernel_data_key.vbprivk" \
    --oldblob "${loop_kern}" \
    --vblockonly

  # Copy the installer vblock to the stateful partition.
  local stateful_dir=$(make_temp_dir)
  sudo mount "${loopdev}p1" "${stateful_dir}"
  sudo cp ${temp_out_vb} ${stateful_dir}/vmlinuz_hd.vblock
  sudo umount "${stateful_dir}"
}

# Do a sanity check on the image's rootfs
# ARGS: Image
verify_image_rootfs() {
  local rootfs=$1
  # This flips the read-only compatibility flag, so that e2fsck does not
  # complain about unknown file system capabilities.
  enable_rw_mount "${rootfs}"
  info "Running e2fsck to check root file system for errors"
  sudo e2fsck -fn "${rootfs}" ||
    die "Root file system has errors!"
  # Flip the bit back so we don't break hashes.
  disable_rw_mount "${rootfs}"
}

# Extracts a firmware updater bundle (for firmware image binaries) file
# (generated by src/platform/firmware/pack_firmware.sh).
# Args: INPUT_FILE OUTPUT_DIR
extract_firmware_bundle() {
  local input="$(readlink -f "$1")"
  local output_dir="$2"
  if [ ! -s "${input}" ]; then
    return 1
  elif grep -q '^##CUTHERE##' "${input}"; then
    # Bundle supports self-extraction.
    "$input" --sb_extract "${output_dir}" ||
      die "Extracting firmware autoupdate (--sb_extract) failed."
  else
    # Legacy bundle - try uudecode.
    uudecode -o - ${input} | tar -C ${output_dir} -zxf - 2>/dev/null ||
      die "Extracting firmware autoupdate failed."
  fi
}

# Repacks firmware updater bundle content from given folder.
# Args: INPUT_DIR TARGET_SCRIPT
repack_firmware_bundle() {
  local input_dir="$1"
  local target="$(readlink -f "$2")"

  if [ ! -s "${target}" ]; then
    return 1
  elif grep -q '^##CUTHERE##' "${target}"; then
    # Bundle supports repacking.
    # Workaround issue crosbug.com/p/33719
    sed -i \
      's/shar -Q -q -x -m -w/shar -Q -q -x -m --no-character-count/' \
      "${target}"
    "$target" --sb_repack "${input_dir}" ||
      die "Updating firmware autoupdate (--sb_repack) failed."
  else
    # Legacy bundle using uuencode + tar.gz.
    # Replace MD5 checksum in the firmware update payload.
    local newfd_checksum="$(md5sum ${input_dir}/bios.bin | cut -f 1 -d ' ')"
    local temp_version="$(make_temp_file)"
    cat ${input_dir}/VERSION |
    sed -e "s#\(.*\)\ \(.*bios.bin.*\)#${newfd_checksum}\ \2#" > ${temp_version}
    mv ${temp_version} ${input_dir}/VERSION

    # Re-generate firmware_update.tgz and copy over encoded archive in
    # the original shell ball.
    sed -ine '/^begin .*firmware_package/,/end/D' "$target"
    tar zcf - -C "${input_dir}" . |
      uuencode firmware_package.tgz >>"${target}"
  fi
}

# Sign a firmware in-place with the given keys.
# Args: FIRMWARE_IMAGE KEY_DIR FIRMWARE_VERSION [LOEM_OUTPUT_DIR]
sign_firmware() {
  local image=$1
  local key_dir=$2
  local firmware_version=$3
  local loem_output_dir=${4:-}

  # Resign the firmware with new keys, also replacing the root and recovery
  # public keys in the GBB.
  "${SCRIPT_DIR}/sign_firmware.sh" "${image}" "${key_dir}" "${image}" \
    "${firmware_version}" "${loem_output_dir}"
  info "Signed firmware image output to ${image}"
}

# Sign nvidia lp0 firmware with the given keys.
# Args: NV_LP0_FIRMWARE_IMAGE KEY_DIR
sign_nv_lp0_firmware() {
  local nv_lp0_fw_image=$1
  local key_dir=$2

  "${SCRIPT_DIR}/sign_nv_cbootimage.sh" "lp0_firmware" \
      "${key_dir%/}/nv_pkc.pem" "${nv_lp0_fw_image}" "tegra210"
  info "Signed nvidia lp0 firmware image output to ${nv_lp0_fw_image}"
}

# Sign a kernel in-place with the given keys.
# Args: KERNEL_IMAGE KEY_DIR KERNEL_VERSION
sign_kernel() {
  local image=$1
  local key_dir=$2
  local kernel_version=$3

  # Note: Although vbutil_kernel may correctly handle specifying the same
  # output file as the input file, we do not want to rely on it correctly
  # handing that. Hence, the use of a temporary file.
  local temp_kernel=$(make_temp_file)

  # Resign the kernel with new keys.
  vbutil_kernel --repack "${temp_kernel}" \
    --keyblock "${key_dir}/kernel.keyblock" \
    --signprivate "${key_dir}/kernel_data_key.vbprivk" \
    --version "${kernel_version}" \
    --oldblob "${image}"

  mv "${temp_kernel}" "${image}"
  info "Signed kernel image output to ${image}"
}

# Sign a recovery kernel in-place with the given keys.
# Args: KERNEL_IMAGE KEY_DIR KERNEL_VERSION
sign_recovery_kernel() {
  local image=$1
  local key_dir=$2
  local kernel_version=$3

  # Note: Although vbutil_kernel may correctly handle specifying the same
  # output file as the input file, we do not want to rely on it correctly
  # handing that. Hence, the use of a temporary file.
  local temp_kernel=$(make_temp_file)

  # Resign the kernel with new recovery keys.
  vbutil_kernel --repack "${temp_kernel}" \
    --keyblock "${key_dir}/recovery_kernel.keyblock" \
    --signprivate "${key_dir}/recovery_kernel_data_key.vbprivk" \
    --version "${kernel_version}" \
    --oldblob "${image}"

  mv "${temp_kernel}" "${image}"
  info "Signed recovery_kernel image output to ${image}"
}

# Sign a delta update payload (usually created by paygen).
# Args: INPUT_IMAGE KEY_DIR OUTPUT_IMAGE
sign_update_payload() {
  local image=$1
  local key_dir=$2
  local output=$3
  local key_size key_file="${key_dir}/update_key.pem"
  # Maps key size to verified boot's algorithm id (for pad_digest_utility).
  # Hashing algorithm is always SHA-256.
  local algo algos=(
    [1024]=1
    [2048]=4
    [4096]=7
    [8192]=10
  )

  key_size=$(openssl rsa -text -noout -in "${key_file}" | \
    sed -n -r '1{s/Private-Key: \(([0-9]*) bit\)/\1/p}')
  algo=${algos[${key_size}]}
  if [[ -z ${algo} ]]; then
    die "Unknown algorithm specified by key_size=${key_size}"
  fi

  pad_digest_utility ${algo} "${image}" | \
    openssl rsautl -sign -pkcs -inkey "${key_file}" -out "${output}"
}

# Re-sign the firmware AU payload inside the image rootfs with a new keys.
# Args: IMAGE
resign_firmware_payload() {
  local image=$1

  if [ -n "${NO_FWUPDATE}" ]; then
    info "Skipping firmware update."
    return
  fi

  # Grab firmware image from the autoupdate bundle (shellball).
  local rootfs_dir=$(make_temp_dir)
  mount_image_partition ${image} 3 ${rootfs_dir}
  local firmware_bundle="${rootfs_dir}/usr/sbin/chromeos-firmwareupdate"
  local shellball_dir=$(make_temp_dir)

  # extract_firmware_bundle can fail if the image has no firmware update.
  if ! extract_firmware_bundle "${firmware_bundle}" "${shellball_dir}"; then
    # Unmount now to prevent changes.
    sudo umount "${rootfs_dir}"
    info "Didn't find a firmware update. Not signing firmware."
    return
  fi
  info "Found a valid firmware update shellball."

  # For context on signing firmware for unified builds, see:
  #   go/cros-unibuild-signing
  #
  # This iterates over a signer_config.csv file, which contains the following:
  #   output_name,image,key_id               (header)
  #   santa,models/santa/bios.bin,SOME_OEM  (sample line)
  #
  # This dictates what output signature blocks to generate based on what
  # keys/binaries.
  #
  # It reuses the LOEM architecture already existing in the signer keysets,
  # but this could be revisited at a future date.
  #
  # Within signer_config.csv, it uses the key_id column to match the key
  # value in loem.ini (if present) and signs the corresponding firmware
  # image using that key.
  #
  # It then outputs the appropriate signature blocks based on the output_name.
  # The firmware updater scripts then detects what output_name to use at
  # runtime based on the platform.
  local signer_config="${shellball_dir}/signer_config.csv"
  if [[ -e "${signer_config}" ]]; then
    info "Using signer_config.csv to determine firmware signatures"
    info "See go/cros-unibuild-signing for details"
    {
      read # Burn the first line (header line)
      while IFS="," read -r output_name bios_image key_id ec_image
      do
        local key_suffix=''
        local extra_args=()
        rootkey="${KEY_DIR}/root_key.vbpubk"

        # If there are OEM specific keys available, we're going to use them.
        # Otherwise, we're going to ignore key_id from the config file and
        # just use the common keys present in the keyset.
        #
        # The presence of the /keyset subdir in the shellball will indicate
        # whether dynamic signature blocks are available or not.
        # This is what updater4.sh currently uses to make the decision.
        if [[ -e "${KEY_DIR}/loem.ini" ]]; then
          # loem.ini has the format KEY_ID_VALUE = KEY_INDEX
          local match="$(grep -E "[0-9]+ = ${key_id}$" "${KEY_DIR}/loem.ini")"
          local key_index="$(echo "${match}" | cut -d ' ' -f 1)"
          info "Detected key index from loem.ini as ${key_index} for ${key_id}"
          if [[ -z "${key_index}" ]]; then
            die "Failed to find key_id ${key_id} in loem.ini file for " \
              "${output_name}"
          fi
          key_suffix=".loem${key_index}"
          shellball_keyset_dir="${shellball_dir}/keyset"
          mkdir -p "${shellball_keyset_dir}"
          extra_args+=(
            --loemdir "${shellball_keyset_dir}"
            --loemid "${output_name}"
          )
          rootkey="${KEY_DIR}/root_key${key_suffix}.vbpubk"
          cp "${rootkey}" "${shellball_keyset_dir}/rootkey.${output_name}"
        fi

        info "Signing firmware image ${bios_image} for ${output_name} " \
          "with key suffix ${key_suffix}"

        local temp_fw=$(make_temp_file)

        local signprivate="${KEY_DIR}/firmware_data_key${key_suffix}.vbprivk"
        local keyblock="${KEY_DIR}/firmware${key_suffix}.keyblock"
        local devsign="${KEY_DIR}/dev_firmware_data_key${key_suffix}.vbprivk"
        local devkeyblock="${KEY_DIR}/dev_firmware${key_suffix}.keyblock"

        if [ ! -e "${devsign}" ] || [ ! -e "${devkeyblock}" ] ; then
          echo "No dev firmware keyblock/datakey found. Reusing normal keys."
          devsign="${signprivate}"
          devkeyblock="${keyblock}"
        fi

        # Path to bios.bin.
        local bios_path="${shellball_dir}/${bios_image}"

        if [ -n "${ec_image}" ]; then
          # Path to ec.bin.
          local ec_path="${shellball_dir}/${ec_image}"

          # Resign ec.bin.
          if is_ec_rw_signed "${ec_path}"; then
            local rw_bin="EC_RW.bin"
            local rw_hash="EC_RW.hash"
            # futility writes byproduct files to CWD, so we cd to temp dir.
            pushd "$(make_temp_dir)" > /dev/null
            ${FUTILITY} sign --type rwsig --prikey \
              "${KEY_DIR}/key_ec_efs.vbprik2" "${ec_path}" \
              || die "Failed to sign ${ec_path}"
            # Above command produces EC_RW.bin. Compute its hash.
            openssl dgst -sha256 -binary "${rw_bin}" > "${rw_hash}"
            # Store EC_RW.bin and its hash in bios.bin.
            store_file_in_cbfs "${bios_path}" "${rw_bin}" "ecrw" \
              || die "Failed to store file in ${bios_path}"
            store_file_in_cbfs "${bios_path}" "${rw_hash}" "ecrw.hash" \
              || die "Failed to store file in ${bios_path}"
            popd > /dev/null
            info "Signed EC image output to ${ec_path}"
          fi
        fi

        # Resign bios.bin.
        ${FUTILITY} sign \
          --signprivate "${signprivate}" \
          --keyblock "${keyblock}" \
          --devsign "${devsign}" \
          --devkeyblock "${devkeyblock}" \
          --kernelkey "${KEY_DIR}/kernel_subkey.vbpubk" \
          --version "${FIRMWARE_VERSION}" \
          "${extra_args[@]}" \
          ${bios_path} \
          ${temp_fw}


        # For development phases, when the GBB can be updated still, set the
        # recovery and root keys in the image.
        ${FUTILITY} gbb \
          -s \
          --recoverykey="${KEY_DIR}/recovery_key.vbpubk" \
          --rootkey="${rootkey}" \
          "${temp_fw}" \
          "${bios_path}"

        info "Signed firmware image output to ${bios_path}"
      done
      unset IFS
    } < "${signer_config}"
  else
    local image_file sign_args=() loem_sfx loem_output_dir
    for image_file in "${shellball_dir}"/bios*.bin; do
      if [[ -e "${KEY_DIR}/loem.ini" ]]; then
        # Extract the extended details from "bios.bin" and use that in the
        # subdir for the keyset.
        loem_sfx=$(sed -r 's:.*/bios([^/]*)[.]bin$:\1:' <<<"${image_file}")
        loem_output_dir="${shellball_dir}/keyset${loem_sfx}"
        sign_args=( "${loem_output_dir}" )
        mkdir -p "${loem_output_dir}"
      fi
      sign_firmware "${image_file}" "${KEY_DIR}" "${FIRMWARE_VERSION}" \
        "${sign_args[@]}"
    done
  fi

  local signer_notes="${shellball_dir}/VERSION.signer"
  echo "" >"$signer_notes"
  echo "Signed with keyset in $(readlink -f "${KEY_DIR}") ." >>"${signer_notes}"
  if [[ -d "${shellball_keyset_dir}"  ]]; then
    echo "List sha1sum of all loem/model's signatures:" >>"${signer_notes}"
    for key in "${shellball_keyset_dir}"/rootkey.*; do
      model="${key##*.}"
      sha1=$(vbutil_key --unpack "${key}" | grep sha1sum | cut -d" " -f9)
      echo "  ${model}: ${sha1}" >>"${signer_notes}"
    done
  fi

  new_shellball=$(make_temp_file)
  cp -f "${firmware_bundle}" "${new_shellball}"
  chmod a+rx "${new_shellball}"
  repack_firmware_bundle "${shellball_dir}" "${new_shellball}"
  sudo cp -f "${new_shellball}" "${firmware_bundle}"
  sudo chmod a+rx "${firmware_bundle}"
  # Unmount now to flush changes.
  sudo umount "${rootfs_dir}"
  info "Re-signed firmware AU payload in ${image}"
}

# Re-sign Android image if exists.
resign_android_image_if_exists() {
  local image=$1

  local rootfs_dir=$(make_temp_dir)
  mount_image_partition "${image}" 3 "${rootfs_dir}"

  local system_img="${rootfs_dir}/opt/google/containers/android/system.raw.img"
  local arc_version=$(grep CHROMEOS_ARC_VERSION= \
    "${rootfs_dir}/etc/lsb-release" | cut -d= -f2)
  if [[ ! -e "${system_img}" || -z "${arc_version}" ]]; then
    info "ARC image not found.  Not signing Android APKs."
    sudo umount "${rootfs_dir}"
    return
  fi

  info "Found ARC image version '${arc_version}', re-signing APKs."
  "${SCRIPT_DIR}/sign_android_image.sh" "${rootfs_dir}" "${KEY_DIR}/android"

  sudo umount "${rootfs_dir}"
  info "Re-signed Android image"
}

# Sign UEFI binaries, if possible.
sign_uefi_binaries() {
  local image="$1"

  if [[ ! -d "${KEY_DIR}/uefi" ]]; then
    return 0
  fi

  local esp_dir="$(mount_image_esp "${image}")"
  if [[ -z "${esp_dir}" ]]; then
    return 0
  elif [[ "${esp_dir}" == "MOUNT_FAILED" ]]; then
    error "Could not mount EFI partition for signing UEFI binaries"
    return 1
  fi
  "${SCRIPT_DIR}/install_gsetup_certs.sh" "${esp_dir}" "${KEY_DIR}/uefi"
  "${SCRIPT_DIR}/sign_uefi.sh" "${esp_dir}" "${KEY_DIR}/uefi"
  sudo umount "${esp_dir}"

  local rootfs_dir="$(make_temp_dir)"
  mount_image_partition "${image}" 3 "${rootfs_dir}"
  "${SCRIPT_DIR}/sign_uefi.sh" "${rootfs_dir}/boot" "${KEY_DIR}/uefi"
  sudo umount "${rootfs_dir}"

  info "Signed UEFI binaries"
  return 0
}

verify_uefi_signatures() {
  local image="$1"
  local succeeded=1

  local esp_dir="$(mount_image_esp "${image}")"
  if [[ -z "${esp_dir}" ]]; then
    return 0
  elif [[ "${esp_dir}" == "MOUNT_FAILED" ]]; then
    error "Could not mount EFI partition for verifying UEFI signatures"
    return 1
  fi
  "${SCRIPT_DIR}/verify_uefi.sh" "${esp_dir}" "${esp_dir}" || succeeded=0

  local rootfs_dir="$(make_temp_dir)"
  mount_image_partition_ro "${image}" 3 "${rootfs_dir}"
  "${SCRIPT_DIR}/verify_uefi.sh" "${rootfs_dir}/boot" "${esp_dir}" || \
      succeeded=0
  sudo umount "${rootfs_dir}"

  sudo umount "${esp_dir}"

  if [[ "${succeeded}" == "0" ]]; then
    die "UEFI signature verification failed"
  fi
}

# Sign an oci container with the given keys.
# Args: CONTAINER KEY_DIR [OUTPUT_CONTAINER]
sign_oci_container() {
  local image=$1
  local key_dir=$2
  local output=$3

  "${SCRIPT_DIR}/sign_oci_container.sh" \
    "${image}" "${key_dir}" --output "${output}"
}

# Verify an image including rootfs hash using the specified keys.
verify_image() {
  local loopdev=$(loopback_partscan "${INPUT_IMAGE}")
  local loop_rootfs="${loopdev}p3"

  info "Verifying RootFS hash..."
  # What we get from image.
  local kernel_config
  # What we calculate from the rootfs.
  local new_kernel_config
  # Depending on the type of image, the verity parameters may
  # exist in either kernel partition 2 or kernel partition 4
  local partnum
  for partnum in 2 4; do
    info "Considering Kernel partition ${partnum}"
    kernel_config=$(sudo dump_kernel_config "${loopdev}p${partnum}")
    local hash_image=$(make_temp_file)
    if ! calculate_rootfs_hash "${loop_rootfs}" "${kernel_config}" \
      "${hash_image}"; then
      info "Trying next kernel partition."
      continue
    fi
    new_kernel_config="$CALCULATED_KERNEL_CONFIG"
    break
  done

  # Note: If calculate_rootfs_hash succeeded above, these should
  # be non-empty.
  expected_hash=$(get_hash_from_config "${new_kernel_config}")
  got_hash=$(get_hash_from_config "${kernel_config}")

  if [ -z "${expected_hash}" ] || [ -z "${got_hash}" ]; then
    die "Couldn't verify RootFS hash on the image."
  fi

  if [ ! "${got_hash}" = "${expected_hash}" ]; then
    cat <<EOF
FAILED: RootFS hash is incorrect.
Expected: ${expected_hash}
Got: ${got_hash}
EOF
    exit 1
  else
    info "PASS: RootFS hash is correct (${expected_hash})"
  fi

  # Now try and verify kernel partition signature.
  set +e
  local try_key=${KEY_DIR}/recovery_key.vbpubk
  info "Testing key verification..."
  # The recovery key is only used in the recovery mode.
  echo -n "With Recovery Key (Recovery Mode ON, Dev Mode OFF): " && \
  { load_kernel_test "${INPUT_IMAGE}" "${try_key}" -b 2 >/dev/null 2>&1 && \
    echo "YES"; } || echo "NO"
  echo -n "With Recovery Key (Recovery Mode ON, Dev Mode ON): " && \
  { load_kernel_test "${INPUT_IMAGE}" "${try_key}" -b 3 >/dev/null 2>&1 && \
    echo "YES"; } || echo "NO"

  try_key=${KEY_DIR}/kernel_subkey.vbpubk
  # The SSD key is only used in non-recovery mode.
  echo -n "With SSD Key (Recovery Mode OFF, Dev Mode OFF): " && \
  { load_kernel_test "${INPUT_IMAGE}" "${try_key}" -b 0 >/dev/null 2>&1  && \
    echo "YES"; } || echo "NO"
  echo -n "With SSD Key (Recovery Mode OFF, Dev Mode ON): " && \
  { load_kernel_test "${INPUT_IMAGE}" "${try_key}" -b 1 >/dev/null 2>&1 && \
    echo "YES"; } || echo "NO"
  set -e

  verify_image_rootfs "${loop_rootfs}"

  verify_uefi_signatures "${INPUT_IMAGE}"

  # TODO(gauravsh): Check embedded firmware AU signatures.
}

# Re-calculate recovery kernel hash.
# Args: LOOPDEV
update_recovery_kernel_hash() {
  local loopdev="$1"
  local loop_kerna="${loopdev}p2"
  local loop_kernb="${loopdev}p4"

  # Update the Kernel B hash in Kernel A command line
  local old_kerna_config="$(sudo dump_kernel_config "${loop_kerna}")"
  local new_kernb_hash=$(sudo sha1sum "${loop_kernb}" | cut -f1 -d' ')

  new_kerna_config=$(make_temp_file)
  echo "$old_kerna_config" |
    sed -e "s#\(kern_b_hash=\)[a-z0-9]*#\1${new_kernb_hash}#" \
      > ${new_kerna_config}
  info "New config for kernel partition 2 is"
  cat ${new_kerna_config}

  # Re-calculate kernel partition signature and command line.
  sudo vbutil_kernel --repack "${loop_kerna}" \
    --keyblock ${KEY_DIR}/recovery_kernel.keyblock \
    --signprivate ${KEY_DIR}/recovery_kernel_data_key.vbprivk \
    --version "${KERNEL_VERSION}" \
    --oldblob "${loop_kerna}" \
    --config ${new_kerna_config}
}

# Update the legacy bootloader templates in EFI partition if available.
# Args: IMAGE_BIN KERNEL
update_legacy_bootloader() {
  local image="$1"
  local loop_kern="$2"

  local esp_dir="$(mount_image_esp "${image}")"
  if [[ -z "${esp_dir}" ]]; then
    info "Not updating legacy bootloader configs: ${image}"
    return 0
  elif [[ "${esp_dir}" == "MOUNT_FAILED" ]]; then
    error "Could not mount EFI partition for updating legacy bootloader cfg."
    return 1
  fi

  # If we can't find the dm parameter in the kernel config, bail out now.
  local kernel_config=$(sudo dump_kernel_config "${loop_kern}")
  local root_hexdigest="$(get_hash_from_config "${kernel_config}")"
  if [[ -z "${root_hexdigest}" ]]; then
    error "Couldn't grab root_digest from kernel partition ${loop_kern}"
    error " (config: ${kernel_config})"
    return 1
  fi
  # Update syslinux configs for legacy BIOS systems.
  if [[ -d "${esp_dir}/syslinux" ]]; then
    local cfg=("${esp_dir}"/syslinux/*.cfg)
    if ! sudo sed -i -r \
      "s/\broot_hexdigest=[a-z0-9]+/root_hexdigest=${root_hexdigest}/g" \
      "${cfg[@]}"; then
        error "Updating syslinux configs failed: '${cfg[*]}'"
        return 1
    fi
  fi
  # Update grub configs for EFI systems.
  local grub_cfg="${esp_dir}/efi/boot/grub.cfg"
  if [[ -f "${grub_cfg}" ]]; then
    if ! sudo sed -i -r \
      "s/\broot_hexdigest=[a-z0-9]+/root_hexdigest=${root_hexdigest}/g" \
      "${grub_cfg}"; then
        error "Updating grub config failed: '${grub_cfg}'"
        return 1
    fi
  fi
}

# Sign an image file with proper keys.
# Args: IMAGE_TYPE INPUT OUTPUT DM_PARTNO KERN_A_KEYBLOCK KERN_A_PRIVKEY \
#       KERN_B_KEYBLOCK KERN_B_PRIVKEY
#
# A ChromiumOS image file (INPUT) always contains 2 partitions (kernel A & B).
# This function will rebuild hash data by DM_PARTNO, resign kernel partitions by
# their KEYBLOCK and PRIVKEY files, and then write to OUTPUT file. Note some
# special images (specified by IMAGE_TYPE, like 'recovery' or 'factory_install')
# may have additional steps (ex, tweaking verity hash or not stripping files)
# when generating output file.
sign_image_file() {
  local image_type="$1"
  local input="$2"
  local output="$3"
  local dm_partno="$4"
  local kernA_keyblock="$5"
  local kernA_privkey="$6"
  local kernB_keyblock="$7"
  local kernB_privkey="$8"

  info "Preparing ${image_type} image..."
  cp --sparse=always "${input}" "${output}"

  local loopdev=$(loopback_partscan "${output}")
  local loop_kern="${loopdev}p${dm_partno}"

  resign_firmware_payload "${output}"
  resign_android_image_if_exists "${output}"
  sign_uefi_binaries "${output}"
  # We do NOT strip /boot for factory installer, since some devices need it to
  # boot EFI. crbug.com/260512 would obsolete this requirement.
  #
  # We also do NOT strip /boot for legacy BIOS or EFI devices.  This is because
  # "cros_installer postinst" on BIOS or EFI systems relies on presence of
  # /boot in rootfs to update kernel.  We infer the BIOS type from the kernel
  # config.
  local loop_kerna="${loopdev}p2"
  local kerna_config="$(sudo dump_kernel_config "${loop_kerna}")"
  if [[ "${image_type}" != "factory_install" &&
        " ${kerna_config} " != *" cros_legacy "* &&
        " ${kerna_config} " != *" cros_efi "* ]]; then
    "${SCRIPT_DIR}/strip_boot_from_image.sh" --image "${output}"
  fi
  update_rootfs_hash "${loopdev}" "${loop_kern}" \
    "${kernA_keyblock}" "${kernA_privkey}" \
    "${kernB_keyblock}" "${kernB_privkey}"
  update_stateful_partition_vblock "${loopdev}"
  if [[ "${image_type}" == "recovery" ]]; then
    update_recovery_kernel_hash "${loopdev}"
  fi
  if ! update_legacy_bootloader "${output}" "${loop_kern}"; then
    # Error is already logged.
    return 1
  fi
  info "Signed ${image_type} image output to ${output}"
}

# Verification
case ${TYPE} in
dump_config)
  check_argc $# 2
  loopdev=$(loopback_partscan "${INPUT_IMAGE}")
  for partnum in 2 4; do
    info "kernel config in partition number ${partnum}:"
    sudo dump_kernel_config "${loopdev}p${partnum}"
    echo
  done
  exit 0
  ;;
verify)
  check_argc $# 2
  verify_image
  exit 0
  ;;
*)
  # All other signing commands take 4 to 5 args.
  if [ -z "${OUTPUT_IMAGE}" ]; then
    # Friendlier message.
    usage "Missing output image name"
  fi
  check_argc $# 4 5
  ;;
esac

# If a version file was specified, read the firmware and kernel
# versions from there.
if [ -n "${VERSION_FILE}" ]; then
  FIRMWARE_VERSION=$(sed -n 's#^firmware_version=\(.*\)#\1#pg' ${VERSION_FILE})
  KERNEL_VERSION=$(sed -n 's#^kernel_version=\(.*\)#\1#pg' ${VERSION_FILE})
fi
info "Using firmware version: ${FIRMWARE_VERSION}"
info "Using kernel version: ${KERNEL_VERSION}"

# Make all modifications on output copy.
if [[ "${TYPE}" == "ssd" || "${TYPE}" == "base" ]]; then
  sign_image_file "SSD" "${INPUT_IMAGE}" "${OUTPUT_IMAGE}" 2 \
    "${KEY_DIR}/kernel.keyblock" "${KEY_DIR}/kernel_data_key.vbprivk" \
    "${KEY_DIR}/kernel.keyblock" "${KEY_DIR}/kernel_data_key.vbprivk"
elif [[ "${TYPE}" == "usb" ]]; then
  sign_image_file "USB" "${INPUT_IMAGE}" "${OUTPUT_IMAGE}" 2 \
    "${KEY_DIR}/recovery_kernel.keyblock" \
    "${KEY_DIR}/recovery_kernel_data_key.vbprivk" \
    "${KEY_DIR}/kernel.keyblock" \
    "${KEY_DIR}/kernel_data_key.vbprivk"
elif [[ "${TYPE}" == "recovery" ]]; then
  sign_image_file "recovery" "${INPUT_IMAGE}" "${OUTPUT_IMAGE}" 4 \
    "${KEY_DIR}/recovery_kernel.keyblock" \
    "${KEY_DIR}/recovery_kernel_data_key.vbprivk" \
    "${KEY_DIR}/kernel.keyblock" \
    "${KEY_DIR}/kernel_data_key.vbprivk"
elif [[ "${TYPE}" == "factory" ]] || [[ "${TYPE}" == "install" ]]; then
  sign_image_file "factory_install" "${INPUT_IMAGE}" "${OUTPUT_IMAGE}" 2 \
    "${KEY_DIR}/installer_kernel.keyblock" \
    "${KEY_DIR}/installer_kernel_data_key.vbprivk" \
    "${KEY_DIR}/kernel.keyblock" \
    "${KEY_DIR}/kernel_data_key.vbprivk"
elif [[ "${TYPE}" == "firmware" ]]; then
  if [[ -e "${KEY_DIR}/loem.ini" ]]; then
    die "LOEM signing not implemented yet for firmware images"
  fi
  cp ${INPUT_IMAGE} ${OUTPUT_IMAGE}
  sign_firmware ${OUTPUT_IMAGE} ${KEY_DIR} ${FIRMWARE_VERSION}
elif [[ "${TYPE}" == "nv_lp0_firmware" ]]; then
  if [[ -e "${KEY_DIR}/loem.ini" ]]; then
    die "LOEM signing not implemented yet for nv_lp0_firmware images"
  fi
  cp "${INPUT_IMAGE}" "${OUTPUT_IMAGE}"
  sign_nv_lp0_firmware "${OUTPUT_IMAGE}" "${KEY_DIR}"
elif [[ "${TYPE}" == "kernel" ]]; then
  if [[ -e "${KEY_DIR}/loem.ini" ]]; then
    die "LOEM signing not implemented yet for kernel images"
  fi
  cp "${INPUT_IMAGE}" "${OUTPUT_IMAGE}"
  sign_kernel "${OUTPUT_IMAGE}" "${KEY_DIR}" "${KERNEL_VERSION}"
elif [[ "${TYPE}" == "recovery_kernel" ]]; then
  if [[ -e "${KEY_DIR}/loem.ini" ]]; then
    die "LOEM signing not implemented yet for recovery_kernel images"
  fi
  cp "${INPUT_IMAGE}" "${OUTPUT_IMAGE}"
  sign_recovery_kernel "${OUTPUT_IMAGE}" "${KEY_DIR}" "${KERNEL_VERSION}"
elif [[ "${TYPE}" == "update_payload" ]]; then
  sign_update_payload ${INPUT_IMAGE} ${KEY_DIR} ${OUTPUT_IMAGE}
elif [[ "${TYPE}" == "accessory_usbpd" ]]; then
  KEY_NAME="${KEY_DIR}/key_$(basename $(dirname ${INPUT_IMAGE}))"
  if [[ ! -e "${KEY_NAME}.pem" ]]; then
    KEY_NAME="${KEY_DIR}/key"
  fi
  cp "${INPUT_IMAGE}" "${OUTPUT_IMAGE}"
  futility sign --type usbpd1 --pem "${KEY_NAME}.pem" "${OUTPUT_IMAGE}"
elif [[ "${TYPE}" == "accessory_rwsig" ]]; then
  KEY_NAME="${KEY_DIR}/key_$(basename $(dirname ${INPUT_IMAGE}))"
  if [[ ! -e "${KEY_NAME}.vbprik2" ]]; then
    KEY_NAME="${KEY_DIR}/key"
  fi
  cp "${INPUT_IMAGE}" "${OUTPUT_IMAGE}"
  futility sign --type rwsig --prikey "${KEY_NAME}.vbprik2" \
           --version "${FIRMWARE_VERSION}" "${OUTPUT_IMAGE}"
elif [[ "${TYPE}" == "oci-container" ]]; then
  sign_oci_container "${INPUT_IMAGE}" "${KEY_DIR}" "${OUTPUT_IMAGE}"
else
  die "Invalid type ${TYPE}"
fi
