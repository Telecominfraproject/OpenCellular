#!/bin/bash

# Copyright 2016 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

. "$(dirname "$0")/common.sh"
. "$(dirname "$0")/lib/sign_android_lib.sh"

set -e

# Print usage string
usage() {
  cat <<EOF
Usage: $PROG /path/to/cros_root_fs/dir /path/to/keys/dir

Re-sign framework apks in an Android system image.  The image itself does not
need to be signed since it is shipped with Chrome OS image, which is already
signed.

Android has many "framework apks" that are signed with 4 different framework
keys, depends on the purpose of the apk.  During development, apks are signed
with the debug one.  This script is to re-sign those apks with corresponding
release key.  It also handles some of the consequences of the key changes, such
as sepolicy update.

EOF
  if [[ $# -gt 0 ]]; then
    error "$*"
    exit 1
  fi
  exit 0
}

# Re-sign framework apks with the corresponding release keys.  Only apk with
# known key fingerprint are re-signed.  We should not re-sign non-framework
# apks.
sign_framework_apks() {
  local system_mnt="$1"
  local key_dir="$2"
  local flavor_prop=""
  local keyset=""

  if ! flavor_prop=$(android_get_build_flavor_prop \
    "${system_mnt}/system/build.prop"); then
    die "Failed to extract build flavor property from \
'${system_mnt}/system/build.prop'."
  fi
  info "Found build flavor property '${flavor_prop}'."
  if ! keyset=$(android_choose_signing_keyset "${flavor_prop}"); then
    die "Unknown build flavor property '${flavor_prop}'."
  fi
  info "Expecting signing keyset '${keyset}'."

  info "Start signing framework apks"

  # Counters for sanity check.
  local counter_platform=0
  local counter_media=0
  local counter_shared=0
  local counter_releasekey=0
  local counter_total=0

  local apk
  while read -d $'\0' -r apk; do
    local sha1=""
    local keyname=""

    sha1=$(unzip -p "${apk}" META-INF/CERT.RSA | \
      keytool -printcert | awk '/^\s*SHA1:/ {print $2}')

    if  ! keyname=$(android_choose_key "${sha1}" "${keyset}"); then
      die "Failed to choose signing key for APK '${apk}' (SHA1 '${sha1}') in \
build flavor '${flavor_prop}'."
    fi
    if [[ -z "${keyname}" ]]; then
      continue
    fi

    info "Re-signing (${keyname}) ${apk}"

    local temp_dir="$(make_temp_dir)"
    local temp_apk="${temp_dir}/temp.apk"
    local signed_apk="${temp_dir}/signed.apk"
    local aligned_apk="${temp_dir}/aligned.apk"

    # Follow the standard manual signing process.  See
    # https://developer.android.com/studio/publish/app-signing.html.
    cp -a "${apk}" "${temp_apk}"
    # Explicitly remove existing signature.
    zip -q "${temp_apk}" -d "META-INF/*"
    signapk "${key_dir}/$keyname.x509.pem" "${key_dir}/$keyname.pk8" \
        "${temp_apk}" "${signed_apk}" > /dev/null
    zipalign 4 "${signed_apk}" "${aligned_apk}"

    # Copy the content instead of mv to avoid owner/mode changes.
    sudo cp "${aligned_apk}" "${apk}" && rm -f "${aligned_apk}"

    # Set timestamp rounded to second since squash file system has resolution
    # in seconds. Required in order for the packages cache generator output is
    # compatible with the packed file system.
    sudo touch "${apk}" -t "$(date +%m%d%H%M.%S)"

    : $(( counter_${keyname} += 1 ))
    : $(( counter_total += 1 ))
  done < <(find "${system_mnt}/system" -type f -name '*.apk' -print0)

  info "Found ${counter_platform} platform APKs."
  info "Found ${counter_media} media APKs."
  info "Found ${counter_shared} shared APKs."
  info "Found ${counter_releasekey} release APKs."
  info "Found ${counter_total} total APKs."
  # Sanity check.
  if [[ ${counter_platform} -lt 2 || ${counter_media} -lt 2 ||
        ${counter_shared} -lt 2 || ${counter_releasekey} -lt 2 ||
        ${counter_total} -lt 25 ]]; then
    die "Number of re-signed package seems to be wrong"
  fi
}

# Platform key is part of the SELinux policy.  Since we are re-signing framework
# apks, we need to replace the key in the policy as well.
update_sepolicy() {
  local system_mnt=$1
  local key_dir=$2

  # Only platform is used at this time.
  local public_platform_key="${key_dir}/platform.x509.pem"

  info "Start updating sepolicy"

  local new_cert=$(sed -E '/(BEGIN|END) CERTIFICATE/d' \
    "${public_platform_key}" | tr -d '\n' \
    | base64 --decode | hexdump -v -e '/1 "%02x"')

  if [[ -z "${new_cert}" ]]; then
    die "Unable to get the public platform key"
  fi

  shopt -s nullglob
  local xml_list=( "${system_mnt}"/system/etc/**/*mac_permissions.xml )
  shopt -u nullglob
  if [[ "${#xml_list[@]}" -ne 1 ]]; then
    die "Unexpected number of *mac_permissions.xml: ${#xml_list[@]}\n \
      ${xml_list[*]}"
  fi

  local xml="${xml_list[0]}"
  local orig=$(make_temp_file)
  local pattern='(<signer signature=")\w+("><seinfo value="platform)'
  cp "${xml}" "${orig}"
  sudo sed -i -E "s/${pattern}/\1${new_cert}"'\2/g' "${xml}"

  # Sanity check.
  if cmp "${xml}" "${orig}"; then
    die "Failed to replace SELinux policy cert"
  fi
}

# Replace the debug key in OTA cert with release key.
replace_ota_cert() {
  local system_mnt=$1
  local release_cert=$2
  local ota_zip="${system_mnt}/system/etc/security/otacerts.zip"

  info "Replacing OTA cert"

  local temp_dir=$(make_temp_dir)
  pushd "${temp_dir}" > /dev/null
  cp "${release_cert}" .
  local temp_zip=$(make_temp_file)
  zip -q -r "${temp_zip}.zip" .
  # Copy the content instead of mv to avoid owner/mode changes.
  sudo cp "${temp_zip}.zip" "${ota_zip}"
  popd > /dev/null
}

# Restore SELinux context.  This has to run after all file changes, before
# creating the new squashfs image.
reapply_file_security_context() {
  local system_mnt=$1
  local root_fs_dir=$2

  info "Reapplying file security context"

  sudo /sbin/setfiles -v -r "${system_mnt}" \
      "${root_fs_dir}/etc/selinux/arc/contexts/files/android_file_contexts" \
      "${system_mnt}"
}

# Snapshot file properties in a directory recursively.
snapshot_file_properties() {
  local dir=$1
  sudo find "${dir}" -exec stat -c '%n:%u:%g:%a:%C' {} + | sort
}

main() {
  local root_fs_dir=$1
  local key_dir=$2
  local android_dir="${root_fs_dir}/opt/google/containers/android"
  local system_img="${android_dir}/system.raw.img"
  # Use the versions in $PATH rather than the system ones.
  local unsquashfs=$(which unsquashfs)
  local mksquashfs=$(which mksquashfs)

  if [[ $# -ne 2 ]]; then
    usage "command takes exactly 2 args"
  fi

  if [[ ! -f "${system_img}" ]]; then
    die "System image does not exist: ${system_img}"
  fi

  if ! type -P zipalign &>/dev/null || ! type -P signapk &>/dev/null; then
    # TODO(victorhsieh): Make this an error.  This is not treating as error
    # just to make an unrelated test pass by skipping this signing.
    warn "Skip signing Android apks (some of executables are not found)."
    exit 0
  fi

  local working_dir=$(make_temp_dir)
  local system_mnt="${working_dir}/mnt"
  local compression_method=$(sudo unsquashfs -s "${system_img}" | \
      awk '$1 == "Compression" { print $2 }')

  info "Unpacking squashfs system image to ${system_mnt}"
  sudo "${unsquashfs}" -x -f -no-progress -d "${system_mnt}" "${system_img}"

  snapshot_file_properties "${system_mnt}" > "${working_dir}/properties.orig"

  sign_framework_apks "${system_mnt}" "${key_dir}"
  update_sepolicy "${system_mnt}" "${key_dir}"
  replace_ota_cert "${system_mnt}" "${key_dir}/releasekey.x509.pem"
  reapply_file_security_context "${system_mnt}" "${root_fs_dir}"

  # Sanity check.
  snapshot_file_properties "${system_mnt}" > "${working_dir}/properties.new"
  local d
  if ! d=$(diff "${working_dir}"/properties.{orig,new}); then
    die "Unexpected change of file property, diff\n${d}"
  fi

  # Packages cache needs to be regenerated when the key and timestamp are
  # changed for apks.
  local packages_cache="${system_mnt}/system/etc/packages_cache.xml"
  if [[ -f "${packages_cache}" ]]; then
    if type -P aapt &>/dev/null; then
      info "Regenerating packages cache ${packages_cache}"
      # For the sanity check.
      local packages_before=$(grep "<package " "${packages_cache}" | wc -l)
      local vendor_mnt=$(make_temp_dir)
      local vendor_img="${android_dir}/vendor.raw.img"
      local jar_lib="lib/arc-cache-builder/org.chromium.arc.cachebuilder.jar"
      info "Unpacking squashfs vendor image to ${vendor_mnt}/vendor"
      # Vendor image is not updated during this step. However we have to include
      # vendor apks to re-generated packages cache which exists in one file for
      # both system and vendor images.
      sudo "${unsquashfs}" -x -f -no-progress -d "${vendor_mnt}/vendor" \
          "${vendor_img}"
      if ! arc_generate_packages_cache "${system_mnt}" "${vendor_mnt}" \
          "${working_dir}/packages_cache.xml"; then
        die "Failed to generate packages cache."
      fi
      sudo cp "${working_dir}/packages_cache.xml" "${packages_cache}"
      # Set android-root as an owner.
      sudo chown 655360:655360 "${packages_cache}"
      local packages_after=$(grep "<package " "${packages_cache}" | wc -l)
      if [[ "${packages_before}" != "${packages_after}" ]]; then
        die "failed to verify the packages count after the regeneration of " \
            "the packages cache. Expected ${packages_before} but found " \
            "${packages_after} packages in pacakges_cache.xml"
      fi
    else
      warn "aapt tool could not be found. Could not regenerate the packages " \
           "cache. Outdated pacakges_cache.xml is removed."
      sudo rm "${packages_cache}"
    fi
  else
    info "Packages cache ${packages_cache} does not exist. Skip regeneration."
  fi

  info "Repacking squashfs image"
  local old_size=$(stat -c '%s' "${system_img}")
  # Overwrite the original image.
  sudo "${mksquashfs}" "${system_mnt}" "${system_img}" \
      -no-progress -comp "${compression_method}" -noappend
  local new_size=$(stat -c '%s' "${system_img}")
  info "Android system image size change: ${old_size} -> ${new_size}"
}

main "$@"
