#!/bin/bash

# Copyright (c) 2010 The Chromium OS Authors. All rights reserved.
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
             install (sign a factory install image)
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
  load_kernel_test dumpe2fs sha1sum e2fsck;
do
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

# Get the hash from a kernel config command line
get_hash_from_config() {
  local kernel_config=$1
  echo ${kernel_config} | sed -e 's/.*dm="\([^"]*\)".*/\1/g' | \
    cut -f2- -d, | cut -f9 -d ' '
}

# Calculate rootfs hash of an image
# Args: ROOTFS_IMAGE KERNEL_CONFIG HASH_IMAGE
#
# rootfs calculation parameters are grabbed from KERNEL_CONFIG
#
# Returns an updated kernel config command line with the new hash.
# and writes the new hash image to the file HASH_IMAGE
calculate_rootfs_hash() {
  local rootfs_image=$1
  local kernel_config=$2
  local hash_image=$3
  local dm_config=$(echo ${kernel_config} |
    sed -e 's/.*dm="\([^"]*\)".*/\1/g' |
    cut -f2- -d,)
  # We extract dm=... portion of the config command line. Here's an example:
  #
  # dm="0 2097152 verity ROOT_DEV HASH_DEV 2097152 1 \
  # sha1 63b7ad16cb9db4b70b28593f825aa6b7825fdcf2"
  #

  if [ -z "${dm_config}" ]; then
    echo "WARNING: Couldn't grab dm_config. Aborting rootfs hash calculation"
    exit 1
  fi
  local rootfs_sectors=$(echo ${dm_config} | cut -f2 -d' ')
  local root_dev=$(echo ${dm_config} | cut -f4 -d ' ')
  local hash_dev=$(echo ${dm_config} | cut -f5 -d ' ')
  local verity_depth=$(echo ${dm_config} | cut -f7 -d' ')
  local verity_algorithm=$(echo ${dm_config} | cut -f8 -d' ')

  # Run the verity tool on the rootfs partition.
  local table="vroot none ro,"$(sudo verity create \
    ${verity_depth} \
    ${verity_algorithm} \
    ${rootfs_image} \
    $((rootfs_sectors / 8)) \
    ${hash_image})
  # Reconstruct new kernel config command line and replace placeholders.
  table="$(echo "$table" |
    sed -s "s|ROOT_DEV|${root_dev}|g;s|HASH_DEV|${hash_dev}|")"
  echo ${kernel_config} | sed -e 's#\(.*dm="\)\([^"]*\)\(".*\)'"#\1${table}\3#g"
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

  # check and clear need_to_resign tag
  local rootfs_dir=$(make_temp_dir)
  mount_image_partition_ro "${image}" 3 "${rootfs_dir}"
  if has_needs_to_be_resigned_tag "${rootfs_dir}"; then
    # remount as RW
    sudo umount -d "${rootfs_dir}"
    mount_image_partition "${image}" 3 "${rootfs_dir}"
    sudo rm -f "${rootfs_dir}/${TAG_NEEDS_TO_BE_SIGNED}"
  fi
  sudo umount -d "${rootfs_dir}"

  local rootfs_image=$(make_temp_file)
  extract_image_partition ${image} 3 ${rootfs_image}
  local kernel_config=$(grab_kernel_config "${image}" ${kernelpart})
  local hash_image=$(make_temp_file)

  # Disable rw mount support prior to hashing.
  disable_rw_mount "${rootfs_image}"

  local new_kernel_config=$(calculate_rootfs_hash "${rootfs_image}" \
    "${kernel_config}" "${hash_image}")
  echo "New config for kernel partition $kernelpart is:"
  echo $new_kernel_config

  local rootfs_blocks=$(sudo dumpe2fs "${rootfs_image}" 2> /dev/null |
    grep "Block count" |
    tr -d ' ' |
    cut -f2 -d:)
  local rootfs_sectors=$((rootfs_blocks * 8))

  # Overwrite the appended hashes in the rootfs
  local temp_config=$(make_temp_file)
  echo ${new_kernel_config} >${temp_config}
  dd if=${hash_image} of=${rootfs_image} bs=512 \
    seek=${rootfs_sectors} conv=notrunc

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

# Extracts the firmware update binaries from the a firmware update
# shell ball (generated by src/platform/firmware/pack_firmware.sh)
# Args: INPUT_SCRIPT OUTPUT_DIR
get_firmwarebin_from_shellball() {
  local input=$1
  local output_dir=$2
  if [ -s "${input}" ]; then
    uudecode -o - ${input} | tar -C ${output_dir} -zxf - 2>/dev/null || \
      { echo "Extracting firmware autoupdate failed." && exit 1; }
  else
    return 1
  fi
}

# Re-sign the firmware AU payload inside the image rootfs with a new keys.
# Args: IMAGE
resign_firmware_payload() {
  local image=$1

  if [ -n "${NO_FWUPDATE}" ]; then
    echo "Skipping firmware update."
    return
  fi

  # Grab firmware image from the autoupdate shellball.
  local rootfs_dir=$(make_temp_dir)
  mount_image_partition ${image} 3 ${rootfs_dir}
  # Force unmount of the rootfs on function exit as it is needed later.
  trap "sudo umount -d ${rootfs_dir}" RETURN

  local shellball_dir=$(make_temp_dir)
  # get_firmwarebin_from_shellball can fail if the image has no
  # firmware update.
  get_firmwarebin_from_shellball \
    ${rootfs_dir}/usr/sbin/chromeos-firmwareupdate ${shellball_dir} || \
    { echo "Didn't find a firmware update. Not signing firmware."
    return; }
  echo "Found a valid firmware update shellball."

  temp_outfd=$(make_temp_file)
  # Replace the root key in the GBB
  # TODO(gauravsh): Remove when we lock down the R/O portion of firmware.
  if [ -e "${KEY_DIR}/hwid" ]; then
    # Only update the hwid if we see one in the key directory.
    gbb_utility -s \
      --rootkey=${KEY_DIR}/root_key.vbpubk \
      --recoverykey=${KEY_DIR}/recovery_key.vbpubk \
      --hwid="$(cat ${KEY_DIR}/hwid)" \
      ${shellball_dir}/bios.bin ${temp_outfd}
  else
    gbb_utility -s \
      --rootkey=${KEY_DIR}/root_key.vbpubk \
      --recoverykey=${KEY_DIR}/recovery_key.vbpubk \
      ${shellball_dir}/bios.bin ${temp_outfd}
  fi
  # Resign the firmware with new keys
  ${SCRIPT_DIR}/resign_firmwarefd.sh ${temp_outfd} ${shellball_dir}/bios.bin \
    ${KEY_DIR}/firmware_data_key.vbprivk \
    ${KEY_DIR}/firmware.keyblock \
    ${KEY_DIR}/dev_firmware_data_key.vbprivk \
    ${KEY_DIR}/dev_firmware.keyblock \
    ${KEY_DIR}/kernel_subkey.vbpubk \
    ${FIRMWARE_VERSION}

  # Replace MD5 checksum in the firmware update payload
  newfd_checksum=$(md5sum ${shellball_dir}/bios.bin | cut -f 1 -d ' ')
  temp_version=$(make_temp_file)
  cat ${shellball_dir}/VERSION |
  sed -e "s#\(.*\)\ \(.*bios.bin.*\)#${newfd_checksum}\ \2#" > ${temp_version}
  sudo cp ${temp_version} ${shellball_dir}/VERSION

  # Re-generate firmware_update.tgz and copy over encoded archive in
  # the original shell ball.
  new_fwblob=$(make_temp_file)
  tar zcf - -C ${shellball_dir} . | \
    uuencode firmware_package.tgz > ${new_fwblob}
  new_shellball=$(make_temp_file)
  cat ${rootfs_dir}/usr/sbin/chromeos-firmwareupdate | \
    sed -e '/^begin .*firmware_package/,/end/D' | \
    cat - ${new_fwblob} >${new_shellball}
  sudo cp ${new_shellball} ${rootfs_dir}/usr/sbin/chromeos-firmwareupdate
  echo "Re-signed firmware AU payload in $image"
}

# Verify an image including rootfs hash using the specified keys.
verify_image() {
  local kernel_config=$(grab_kernel_config ${INPUT_IMAGE} 2)
  local rootfs_image=$(make_temp_file)
  extract_image_partition ${INPUT_IMAGE} 3 ${rootfs_image}
  local hash_image=$(make_temp_file)
  local type=""

  # First, perform RootFS verification
  echo "Verifying RootFS hash..."
  local new_kernel_config=$(calculate_rootfs_hash "${rootfs_image}" \
    "${kernel_config}" "${hash_image}")
  local expected_hash=$(get_hash_from_config "${new_kernel_config}")
  local got_hash=$(get_hash_from_config "${kernel_config}")

  if [ -z "${expected_hash}" ]; then
    echo "FAILED: RootFS hash is empty!"
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
  # Update the Kernel B hash in Kernel A command line
  temp_kimageb=$(make_temp_file)
  extract_image_partition ${image_bin} 4 ${temp_kimageb}
  local kern_a_config=$(grab_kernel_config "${image_bin}" 2)
  local kern_b_hash=$(sha1sum ${temp_kimageb} | cut -f1 -d' ')

  temp_configa=$(make_temp_file)
  echo "$kern_a_config" |
    sed -e "s#\(kern_b_hash=\)[a-z0-9]*#\1${kern_b_hash}#" > ${temp_configa}
  echo "New config for kernel partition 2 is"
  cat $temp_configa

  local temp_kimagea=$(make_temp_file)
  extract_image_partition ${image_bin} 2 ${temp_kimagea}
  # Re-calculate kernel partition signature and command line.
  local updated_kimagea=$(make_temp_file)
  vbutil_kernel --repack ${updated_kimagea} \
    --keyblock ${KEY_DIR}/recovery_kernel.keyblock \
    --signprivate ${KEY_DIR}/recovery_kernel_data_key.vbprivk \
    --version "${KERNEL_VERSION}" \
    --oldblob ${temp_kimagea} \
    --config ${temp_configa}

  replace_image_partition ${image_bin} 2 ${updated_kimagea}

  # Now generate the installer vblock with the SSD keys.
  # The installer vblock is for KERN-B on recovery images.
  temp_out_vb=$(make_temp_file)
  extract_image_partition ${image_bin} 4 ${temp_kimageb}
  ${SCRIPT_DIR}/resign_kernel_partition.sh ${temp_kimageb} ${temp_out_vb} \
    ${KEY_DIR}/kernel_data_key.vbprivk \
    ${KEY_DIR}/kernel.keyblock \
    "${KERNEL_VERSION}"

  # Copy the installer vblock to the stateful partition.
  # TODO(gauravsh): Remove this if we get rid of the need to overwrite
  # the vblock during installs. Kern B could directly be signed by the
  # SSD keys.
  # Note: This vblock is also needed for the ability to convert a recovery
  # image into the equivalent SSD image (convert_recovery_to_ssd.sh)
  local stateful_dir=$(make_temp_dir)
  mount_image_partition ${image_bin} 1 ${stateful_dir}
  sudo cp ${temp_out_vb} ${stateful_dir}/vmlinuz_hd.vblock

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
elif [ "${TYPE}" == "install" ]; then
  cp ${INPUT_IMAGE} ${OUTPUT_IMAGE}
  resign_firmware_payload ${OUTPUT_IMAGE}
  update_rootfs_hash ${OUTPUT_IMAGE} \
    ${KEY_DIR}/installer_kernel.keyblock \
    ${KEY_DIR}/installer_kernel_data_key.vbprivk \
    2
  sign_for_factory_install ${OUTPUT_IMAGE}
else
  echo "Invalid type ${TYPE}"
  exit 1
fi
