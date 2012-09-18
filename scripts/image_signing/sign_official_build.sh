#!/bin/bash

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Sign the final build image using the "official" keys.
#
# Prerequisite tools needed in the system path:
#
#  gbb_utility (from src/platform/vboot_reference)
#  vbutil_kernel (from src/platform/vboot_reference)
#  cgpt (from src/platform/vboot_reference)
#  dump_kernel_config (from src/platform/vboot_reference)
#  verity (from src/platform/verity)
#  load_kernel_test (from src/platform/vboot_reference)
#  dumpe2fs
#  sha1sum

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

# Print usage string
usage() {
  cat <<EOF
Usage: $PROG <type> input_image /path/to/keys/dir [output_image] [version_file]
where <type> is one of:
             ssd  (sign an SSD image)
             recovery (sign a USB recovery image)
             factory (sign a factory install image)
             install (old alias to "factory")
             update_payload (sign a delta update hash)
             firmware (sign a firmware image)
             usb  (sign an image to boot directly from USB)
             verify (verify an image including rootfs hashes)

output_image: File name of the signed output image
version_file: File name of where to read the kernel and firmware versions.

If you are signing an image, you must specify an [output_image] and
optionally, a [version_file].

EOF
}

if [ $# -lt 3 ] || [ $# -gt 5 ]; then
  usage
  exit 1
fi

# Abort on errors.
set -e

# Add to the path since some tools reside here and may not be in the non-root
# system path.
PATH=$PATH:/usr/sbin:/sbin

# Make sure the tools we need are available.
for prereqs in gbb_utility vbutil_kernel cgpt dump_kernel_config verity \
  load_kernel_test dumpe2fs sha1sum e2fsck; do
  type -P "${prereqs}" &>/dev/null || \
    { echo "${prereqs} tool not found."; exit 1; }
done

TYPE=$1
INPUT_IMAGE=$2
KEY_DIR=$3
OUTPUT_IMAGE=$4
VERSION_FILE=$5

FIRMWARE_VERSION=1
KERNEL_VERSION=1

# Get current rootfs hash and kernel command line
# ARGS: IMAGE KERNELPART
grab_kernel_config() {
  local image=$1
  local kernelpart=$2  # Kernel partition number to grab.
  # Grab the existing kernel partition and get the kernel config.
  temp_kimage=$(make_temp_file)
  extract_image_partition ${image} ${kernelpart} ${temp_kimage}
  dump_kernel_config ${temp_kimage}
}

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
  echo ${kernel_config} | sed -ne 's/.*dm="\([^"]*\)".*/\1/gp' | cut -f2- -d,
}
# Get the verity root digest hash from a kernel config command line.
get_hash_from_config() {
  local kernel_config=$1
  local dm_config=$(get_dmparams_from_config "${kernel_config}")
  if is_old_verity_argv "${dm_config}"; then
    echo ${dm_config} | cut -f9 -d ' '
  else
    echo $(get_verity_arg "${dm_config}" root_hexdigest)
  fi
}

CALCULATED_KERNEL_CONFIG=
# Calculate rootfs hash of an image
# Args: ROOTFS_IMAGE KERNEL_CONFIG HASH_IMAGE
#
# rootfs calculation parameters are grabbed from KERNEL_CONFIG
#
# Updated kernel config command line with the new hash is stored in
# $CALCULATED_KERNEL_CONFIG and the new hash image is written to the file
# HASH_IMAGE.
calculate_rootfs_hash() {
  local rootfs_image=$1
  local kernel_config=$2
  local hash_image=$3
  local dm_config=$(get_dmparams_from_config "${kernel_config}")

  if [ -z "${dm_config}" ]; then
    echo "WARNING: Couldn't grab dm_config. Aborting rootfs hash calculation."
    return 1
  fi

  local rootfs_sectors
  local verity_depth
  local verity_algorithm
  local root_dev
  local hash_dev
  local verity_bin="verity"
  if is_old_verity_argv "${dm_config}"; then
    # dm="0 2097152 verity ROOT_DEV HASH_DEV 2097152 1 \
    # sha1 63b7ad16cb9db4b70b28593f825aa6b7825fdcf2"
    rootfs_sectors=$(echo ${dm_config} | cut -f2 -d' ')
    verity_depth=$(echo ${dm_config} | cut -f7 -d' ')
    verity_algorithm=$(echo ${dm_config} | cut -f8 -d' ')
    root_dev=$(echo ${dm_config} | cut -f4 -d ' ')
    hash_dev=$(echo ${dm_config} | cut -f5 -d ' ')
    # Hack around the fact that the signer needs to use the old version of
    # verity to generate legacy verity kernel parameters. If we find it,
    # we use it.
    type -P "verity-old" &>/dev/null && verity_bin="verity-old"
  else
    # Key-value parameters.
    rootfs_sectors=$(get_verity_arg "${dm_config}" hashstart)
    verity_depth=0
    verity_algorithm=$(get_verity_arg "${dm_config}" alg)
    root_dev=$(get_verity_arg "${dm_config}" payload)
    hash_dev=$(get_verity_arg "${dm_config}" hashtree)
    salt=$(get_verity_arg "${dm_config}" salt)
  fi

  local salt_arg
  if [ -n "$salt" ]; then
    salt_arg="salt=$salt"
  fi

  # Run the verity tool on the rootfs partition.
  local table="vroot none ro,"$(sudo ${verity_bin} mode=create \
    alg=${verity_algorithm} \
    payload="${rootfs_image}" \
    payload_blocks=$((rootfs_sectors / 8)) \
    hashtree="${hash_image}" ${salt_arg})
  # Reconstruct new kernel config command line and replace placeholders.
  table="$(echo "$table" |
    sed -s "s|ROOT_DEV|${root_dev}|g;s|HASH_DEV|${hash_dev}|")"
  CALCULATED_KERNEL_CONFIG=$(echo ${kernel_config} |
    sed -e 's#\(.*dm="\)\([^"]*\)\(".*\)'"#\1${table}\3#g")
}

# Re-calculate rootfs hash, update rootfs and kernel command line.
# Args: IMAGE KEYBLOCK PRIVATEKEY KERNELPART
update_rootfs_hash() {
  local image=$1  # Input image.
  local keyblock=$2  # Keyblock for re-generating signed kernel partition
  local signprivate=$3  # Private key to use for signing.
  local kernelpart=$4  # Kernel partition number to update (usually 2 or 4)

  echo "Updating rootfs hash and updating config for Kernel partition" \
    "$kernelpart"

  # If we can't find dm parameters in the kernel config, bail out now.
  local kernel_config=$(grab_kernel_config "${image}" ${kernelpart})
  local dm_config=$(get_dmparams_from_config "${kernel_config}")
  if [ -z "${dm_config}" ]; then
    echo "WARNING: Couldn't grab dm_config from kernel partition ${kernelpart}"
    echo "WARNING: Not performing rootfs hash update!"
    return
  fi

  # check and clear need_to_resign tag
  local rootfs_dir=$(make_temp_dir)
  mount_image_partition_ro "${image}" 3 "${rootfs_dir}"
  if has_needs_to_be_resigned_tag "${rootfs_dir}"; then
    # remount as RW
    sudo umount "${rootfs_dir}"
    mount_image_partition "${image}" 3 "${rootfs_dir}"
    sudo rm -f "${rootfs_dir}/${TAG_NEEDS_TO_BE_SIGNED}"
  fi
  sudo umount "${rootfs_dir}"

  local rootfs_image=$(make_temp_file)
  extract_image_partition ${image} 3 ${rootfs_image}
  local hash_image=$(make_temp_file)

  # Disable rw mount support prior to hashing.
  disable_rw_mount "${rootfs_image}"

  if ! calculate_rootfs_hash "${rootfs_image}"  "${kernel_config}" \
    "${hash_image}"; then
    echo "calculate_rootfs_hash failed!"
    echo "Aborting rootfs hash update!"
    return
  fi

  local new_kernel_config=$CALCULATED_KERNEL_CONFIG
  echo "New config for kernel partition $kernelpart is:"
  echo $new_kernel_config
  echo

  local rootfs_blocks=$(sudo dumpe2fs "${rootfs_image}" 2> /dev/null |
    grep "Block count" |
    tr -d ' ' |
    cut -f2 -d:)
  local rootfs_sectors=$((rootfs_blocks * 8))

  # Overwrite the appended hashes in the rootfs
  local temp_config=$(make_temp_file)
  echo ${new_kernel_config} >${temp_config}
  dd if=${hash_image} of=${rootfs_image} bs=512 \
    seek=${rootfs_sectors} conv=notrunc 2>/dev/null

  local temp_kimage=$(make_temp_file)
  extract_image_partition ${image} ${kernelpart} ${temp_kimage}
  # Re-calculate kernel partition signature and command line.
  local updated_kimage=$(make_temp_file)
  vbutil_kernel --repack ${updated_kimage} \
    --keyblock ${keyblock} \
    --signprivate ${signprivate} \
    --version "${KERNEL_VERSION}" \
    --oldblob ${temp_kimage} \
    --config ${temp_config}

  replace_image_partition ${image} ${kernelpart} ${updated_kimage}
  replace_image_partition ${image} 3 ${rootfs_image}
}

# Do a sanity check on the image's rootfs
# ARGS: Image
verify_image_rootfs() {
  local image=$1
  local rootfs_image=$(make_temp_file)
  extract_image_partition ${image} 3 ${rootfs_image}
  # This flips the read-only compatibility flag, so that e2fsck does not
  # complain about unknown file system capabilities.
  enable_rw_mount ${rootfs_image}
  echo "Running e2fsck to check root file system for errors"
  sudo e2fsck -fn "${rootfs_image}" ||
    { echo "Root file system has errors!" && exit 1;}
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
# Args: FIRMWARE_IMAGE KEY_DIR FIRMWARE_VERSION
sign_firmware() {
  local image=$1
  local key_dir=$2
  local firmware_version=$3

  local temp_firmware=$(make_temp_file)
  # Resign the firmware with new keys, also replacing the root and recovery
  # public keys in the GBB.
  ${SCRIPT_DIR}/sign_firmware.sh ${image} ${key_dir} ${temp_firmware} \
    ${firmware_version}
  # Note: Although sign_firmware.sh may correctly handle specifying the same
  # output file as the input file, we do not want to rely on it correctly
  # handing that. Hence, the use of a temporary file.
  mv ${temp_firmware} ${image}
  echo "Signed firmware image output to ${image}"
}

# Sign a delta update payload (usually created by paygen).
# Args: INPUT_IMAGE KEY_DIR OUTPUT_IMAGE
sign_update_payload() {
  local image=$1
  local key_dir=$2
  local output=$3
  local key_size key_file="${key_dir}/update_key.pem"
  local algo algos=(
    # Maps key size to verified boot's algorithm id (for pad_digest_utility).
    # Hashing algorithm is always SHA-256.
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
    echo "Skipping firmware update."
    return
  fi

  # Grab firmware image from the autoupdate bundle (shellball).
  local rootfs_dir=$(make_temp_dir)
  mount_image_partition ${image} 3 ${rootfs_dir}
  # Force unmount of the rootfs on function exit as it is needed later.
  trap "sudo umount ${rootfs_dir}" RETURN
  local firmware_bundle="${rootfs_dir}/usr/sbin/chromeos-firmwareupdate"
  local shellball_dir=$(make_temp_dir)

  # extract_firmware_bundle can fail if the image has no firmware update.
  extract_firmware_bundle "${firmware_bundle}" "${shellball_dir}" ||
    { echo "Didn't find a firmware update. Not signing firmware."
    return; }
  echo "Found a valid firmware update shellball."

  sign_firmware ${shellball_dir}/bios.bin ${KEY_DIR} ${FIRMWARE_VERSION}

  local signer_notes="${shellball_dir}/VERSION.signer"
  echo "" >"$signer_notes"
  echo "Signed with keyset in $(readlink -f "${KEY_DIR}") ." >>"$signer_notes"

  new_shellball=$(make_temp_file)
  cp -f "${firmware_bundle}" "${new_shellball}"
  chmod a+rx "${new_shellball}"
  repack_firmware_bundle "${shellball_dir}" "${new_shellball}"
  sudo cp -f "${new_shellball}" "${firmware_bundle}"
  sudo chmod a+rx "${firmware_bundle}"
  echo "Re-signed firmware AU payload in $image"
}

# Verify an image including rootfs hash using the specified keys.
verify_image() {
  local rootfs_image=$(make_temp_file)
  extract_image_partition ${INPUT_IMAGE} 3 ${rootfs_image}

  echo "Verifying RootFS hash..."
  # What we get from image.
  local kernel_config
  # What we calculate from the rootfs.
  local new_kernel_config
  # Depending on the type of image, the verity parameters may
  # exist in either kernel partition 2 or kernel partition 4
  local partnum
  for partnum in 2 4; do
    echo "Considering Kernel partition $partnum"
    kernel_config=$(grab_kernel_config ${INPUT_IMAGE} $partnum)
    local hash_image=$(make_temp_file)
    if ! calculate_rootfs_hash "${rootfs_image}" "${kernel_config}" \
      "${hash_image}"; then
      echo "Trying next kernel partition."
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
    echo "FAILURE: Couldn't verify RootFS hash on the image."
    exit 1
  fi

  if [ ! "${got_hash}" = "${expected_hash}" ]; then
    cat <<EOF
FAILED: RootFS hash is incorrect.
Expected: ${expected_hash}
Got: ${got_hash}
EOF
    exit 1
  else
    echo "PASS: RootFS hash is correct (${expected_hash})"
  fi

  # Now try and verify kernel partition signature.
  set +e
  local try_key=${KEY_DIR}/recovery_key.vbpubk
  echo "Testing key verification..."
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

  verify_image_rootfs "${INPUT_IMAGE}"

  # TODO(gauravsh): Check embedded firmware AU signatures.
}

# Sign the kernel partition on an image using the given keys. Modifications are
# made in-place.
# Args: src_bin kernel_datakey kernel_keyblock kernel_version
sign_image_inplace() {
  src_bin=$1
  kernel_datakey=$2
  kernel_keyblock=$3
  kernel_version=$4

  temp_kimage=$(make_temp_file)
  extract_image_partition ${src_bin} 2 ${temp_kimage}
  updated_kimage=$(make_temp_file)

  vbutil_kernel --repack "${updated_kimage}" \
  --keyblock "${kernel_keyblock}" \
  --signprivate "${kernel_datakey}" \
  --version "${kernel_version}" \
  --oldblob "${temp_kimage}"
  replace_image_partition ${src_bin} 2 ${updated_kimage}
}

# Generate the SSD image
# Args: image_bin
sign_for_ssd() {
  image_bin=$1
  sign_image_inplace ${image_bin} ${KEY_DIR}/kernel_data_key.vbprivk \
    ${KEY_DIR}/kernel.keyblock \
    "${KERNEL_VERSION}"
  echo "Signed SSD image output to ${image_bin}"
}

# Generate the USB image (direct boot)
sign_for_usb() {
  image_bin=$1
  sign_image_inplace ${image_bin} ${KEY_DIR}/recovery_kernel_data_key.vbprivk \
    ${KEY_DIR}/recovery_kernel.keyblock \
    "${KERNEL_VERSION}"

  # Now generate the installer vblock with the SSD keys.
  # The installer vblock is for KERN-A on direct boot images.
  temp_kimagea=$(make_temp_file)
  temp_out_vb=$(make_temp_file)
  extract_image_partition ${image_bin} 2 ${temp_kimagea}
  ${SCRIPT_DIR}/resign_kernel_partition.sh ${temp_kimagea} ${temp_out_vb} \
    ${KEY_DIR}/kernel_data_key.vbprivk \
    ${KEY_DIR}/kernel.keyblock \
    "${KERNEL_VERSION}"

  # Copy the installer vblock to the stateful partition.
  local stateful_dir=$(make_temp_dir)
  mount_image_partition ${image_bin} 1 ${stateful_dir}
  sudo cp ${temp_out_vb} ${stateful_dir}/vmlinuz_hd.vblock

  echo "Signed USB image output to ${image_bin}"
}

# Generate the USB (recovery + install) image
# Args: image_bin
sign_for_recovery() {
  image_bin=$1

  # Sign the install kernel with SSD keys.
  local temp_kimageb=$(make_temp_file)
  extract_image_partition ${image_bin} 4 ${temp_kimageb}
  local updated_kimageb=$(make_temp_file)
  vbutil_kernel --repack ${updated_kimageb} \
    --keyblock ${KEY_DIR}/kernel.keyblock \
    --signprivate ${KEY_DIR}/kernel_data_key.vbprivk \
    --version "${KERNEL_VERSION}" \
    --oldblob ${temp_kimageb}

  replace_image_partition ${image_bin} 4 ${updated_kimageb}

  # Copy the SSD kernel vblock to the stateful partition.
  # TODO(gauravsh): Get rid of this once --skip_vblock is nuked from
  # orbit everywhere. crosbug.com/8378
  local temp_out_vb=$(make_temp_file)
  ${SCRIPT_DIR}/resign_kernel_partition.sh ${temp_kimageb} ${temp_out_vb} \
    ${KEY_DIR}/kernel_data_key.vbprivk \
    ${KEY_DIR}/kernel.keyblock \
    "${KERNEL_VERSION}"
  local stateful_dir=$(make_temp_dir)
  mount_image_partition ${image_bin} 1 ${stateful_dir}
  sudo cp ${temp_out_vb} ${stateful_dir}/vmlinuz_hd.vblock
  sudo umount "${stateful_dir}"

  # Update the Kernel B hash in Kernel A command line
  local old_kerna_config=$(grab_kernel_config "${image_bin}" 2)
  local new_kernb=$(make_temp_file)
  # Can't use updated_kimageb since the hash is calculated on the
  # whole partition including the null padding at the end.
  extract_image_partition ${image_bin} 4 ${new_kernb}
  local new_kernb_hash=$(sha1sum ${new_kernb} | cut -f1 -d' ')

  new_kerna_config=$(make_temp_file)
  echo "$old_kerna_config" |
    sed -e "s#\(kern_b_hash=\)[a-z0-9]*#\1${new_kernb_hash}#" \
      > ${new_kerna_config}
  echo "New config for kernel partition 2 is"
  cat ${new_kerna_config}

  local temp_kimagea=$(make_temp_file)
  extract_image_partition ${image_bin} 2 ${temp_kimagea}

  # Re-calculate kernel partition signature and command line.
  local updated_kimagea=$(make_temp_file)
  vbutil_kernel --repack ${updated_kimagea} \
    --keyblock ${KEY_DIR}/recovery_kernel.keyblock \
    --signprivate ${KEY_DIR}/recovery_kernel_data_key.vbprivk \
    --version "${KERNEL_VERSION}" \
    --oldblob ${temp_kimagea} \
    --config ${new_kerna_config}

  replace_image_partition ${image_bin} 2 ${updated_kimagea}
  echo "Signed recovery image output to ${image_bin}"
}

# Generate the factory install image.
# Args: image_bin
sign_for_factory_install() {
  image_bin=$1
  sign_image_inplace ${image_bin} ${KEY_DIR}/installer_kernel_data_key.vbprivk \
    ${KEY_DIR}/installer_kernel.keyblock \
    "${KERNEL_VERSION}"
  echo "Signed factory install image output to ${image_bin}"
}

# Verification
if [ "${TYPE}" == "verify" ]; then
  verify_image
  exit 0
fi

# Signing requires an output image name
if [ -z "${OUTPUT_IMAGE}" ]; then
  usage
  exit 1
fi

# If a version file was specified, read the firmware and kernel
# versions from there.
if [ -n "${VERSION_FILE}" ]; then
  FIRMWARE_VERSION=$(sed -n 's#^firmware_version=\(.*\)#\1#pg' ${VERSION_FILE})
  KERNEL_VERSION=$(sed -n 's#^kernel_version=\(.*\)#\1#pg' ${VERSION_FILE})
fi
echo "Using firmware version: ${FIRMWARE_VERSION}"
echo "Using kernel version: ${KERNEL_VERSION}"

# Make all modifications on output copy.
if [ "${TYPE}" == "ssd" ]; then
  cp ${INPUT_IMAGE} ${OUTPUT_IMAGE}
  resign_firmware_payload ${OUTPUT_IMAGE}
  update_rootfs_hash ${OUTPUT_IMAGE} \
    ${KEY_DIR}/kernel.keyblock \
    ${KEY_DIR}/kernel_data_key.vbprivk \
    2
  sign_for_ssd ${OUTPUT_IMAGE}
elif [ "${TYPE}" == "usb" ]; then
  cp ${INPUT_IMAGE} ${OUTPUT_IMAGE}
  resign_firmware_payload ${OUTPUT_IMAGE}
  update_rootfs_hash ${OUTPUT_IMAGE} \
    ${KEY_DIR}/recovery_kernel.keyblock \
    ${KEY_DIR}/recovery_kernel_data_key.vbprivk \
    2
  sign_for_usb ${OUTPUT_IMAGE}
elif [ "${TYPE}" == "recovery" ]; then
  cp ${INPUT_IMAGE} ${OUTPUT_IMAGE}
  resign_firmware_payload ${OUTPUT_IMAGE}
  # Both kernel command lines must have the correct rootfs hash
  update_rootfs_hash ${OUTPUT_IMAGE} \
    ${KEY_DIR}/recovery_kernel.keyblock \
    ${KEY_DIR}/recovery_kernel_data_key.vbprivk \
    4
  update_rootfs_hash ${OUTPUT_IMAGE} \
    ${KEY_DIR}/recovery_kernel.keyblock \
    ${KEY_DIR}/recovery_kernel_data_key.vbprivk \
    2
  sign_for_recovery ${OUTPUT_IMAGE}
elif [ "${TYPE}" == "factory" ] || [ "${TYPE}" == "install" ]; then
  cp ${INPUT_IMAGE} ${OUTPUT_IMAGE}
  resign_firmware_payload ${OUTPUT_IMAGE}
  update_rootfs_hash ${OUTPUT_IMAGE} \
    ${KEY_DIR}/installer_kernel.keyblock \
    ${KEY_DIR}/installer_kernel_data_key.vbprivk \
    2
  sign_for_factory_install ${OUTPUT_IMAGE}
elif [ "${TYPE}" == "firmware" ]; then
  cp ${INPUT_IMAGE} ${OUTPUT_IMAGE}
  sign_firmware ${OUTPUT_IMAGE} ${KEY_DIR} ${FIRMWARE_VERSION}
elif [ "${TYPE}" == "update_payload" ]; then
  sign_update_payload ${INPUT_IMAGE} ${KEY_DIR} ${OUTPUT_IMAGE}
else
  echo "Invalid type ${TYPE}"
  exit 1
fi
