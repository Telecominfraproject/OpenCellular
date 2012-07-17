#!/bin/bash

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Script to manipulate the tag files in the output of build_image

# Load common constants.  This should be the first executable line.
# The path to common.sh should be relative to your script's location.
. "$(dirname "$0")/common.sh"

load_shflags

DEFINE_string from "chromiumos_image.bin" \
  "Input file name of Chrome OS image to tag/stamp."
DEFINE_string dev_mode "" \
  "(build-info) Tag as a developer mode build (1 to enable, 0 to disable)"
DEFINE_string update_firmware "" \
  "(auto-update) Force updating firmware (1 to enable, 0 to disable)"
DEFINE_string leave_firmware_alone "" \
  "(auto-update) For BIOS development use ONLY (1 to enable, 0 to disable)"
DEFINE_string leave_core "" \
  "(crash-reporter) Leave core dumps (1 to enable, 0 to disable)"
DEFINE_string crosh_workarounds "" \
  "(crosh) Keep crosh (1 to keep, 0 to disable *irreversible*)"

# Parameters for manipulating /etc/lsb-release.
DEFINE_boolean remove_test_label false \
  "(build-info) Remove 'test' suffix in /etc/lsb-release"
DEFINE_boolean change_dev_to_beta false \
  "(build-info) Change 'dev' -> 'beta' in /etc/lsb-release"


# TODO(hungte) we can add factory_installer and factory_test,
# but I don't see any reason to tweak/check these values.

# Parse command line.
FLAGS "$@" || exit 1
eval set -- "${FLAGS_ARGV}"

# Abort on error.
set -e

if [ -z "${FLAGS_from}" ] || [ ! -s "${FLAGS_from}" ] ; then
  echo "Error: need a valid file by --from"
  exit 1
fi

# Global variable to track if the image is modified.
g_modified=${FLAGS_FALSE}

# Processes (enable, disable, or simply report) a tag file.
# Args: DO_MODIFICATION NAME ROOT TAG_FILE ACTION
#
# When DO_MODIFICATION=${FLAGS_TRUE},
#  Creates (ACTION=1) the TAG_FILE in ROOT, or
#  removes (ACTION=0) the TAG_FILE in ROOT, then
#  reports the status (and change) to the tag file.
# When DO_MODIFICATION=${FLAGS_FALSE},
#  make a dry-run and only change ${g_modified}
function process_tag() {
  local tag_status_text=""
  local do_modification="$1"
  local name="$2"
  local root="$3"
  local tag_file_path="$3/$4"
  local action="$5"
  local do_enable=${FLAGS_FALSE}
  local do_disable=${FLAGS_FALSE}

  # only 1, 0, and "" are valid params to action.
  case "${action}" in
    "1" )
      do_enable=${FLAGS_TRUE}
      ;;
    "0" )
      do_disable=${FLAGS_TRUE}
      ;;
    "" )
      ;;
    * )
      echo "Error: invalid param to ${name}: ${action} (must be 1 or 0)."
      exit 1
      ;;
  esac

  if [ -f "${tag_file_path}" ]; then
    tag_status_text="ENABLED"
    if [ "${do_disable}" = ${FLAGS_TRUE} ]; then
      # disable the tag
      if [ "${do_modification}" = ${FLAGS_TRUE} ]; then
        sudo rm "${tag_file_path}"
      fi
      g_modified=${FLAGS_TRUE}
      tag_status_text="${tag_status_text} => disabled"
    elif [ "${do_disable}" != ${FLAGS_FALSE} ]; then
      # internal error
      echo "Internal error for tag ${name}: need disable param." 1>&2
      exit 1
    fi
  else
    tag_status_text="disabled"
    if [ "${do_enable}" = ${FLAGS_TRUE} ]; then
      # enable the tag
      if [ "${do_modification}" = ${FLAGS_TRUE} ]; then
        sudo touch "${tag_file_path}"
      fi
      g_modified=${FLAGS_TRUE}
      tag_status_text="${tag_status_text} => ENABLED"
    elif [ "${do_enable}" != ${FLAGS_FALSE} ]; then
      # internal error
      echo "Internal error for tag ${name}: need enable param." 1>&2
      exit 1
    fi
  fi

  # report tag status
  if [ "${do_modification}" != ${FLAGS_TRUE} ]; then
    echo "${name}: ${tag_status_text}"
  fi
}

# Iterates all tags to a given partition root.
# Args: ROOTFS DO_MODIFICATION
#
# Check process_tag for the meaning of parameters.
process_all_tags() {
  local rootfs="$1"
  local do_modification="$2"

  process_tag "${do_modification}" \
    "(build-info) dev_mode" \
    "${rootfs}" \
    /root/.dev_mode \
    "${FLAGS_dev_mode}"

  process_tag "${do_modification}" \
    "(auto-update) update_firmware" \
    "${rootfs}" \
    /root/.force_update_firmware \
    "${FLAGS_update_firmware}"

  process_tag "${do_modification}" \
    "(auto-update) leave_firmware_alone" \
    "${rootfs}" \
    /root/.leave_firmware_alone \
    "${FLAGS_leave_firmware_alone}"

  process_tag "${do_modification}" \
    "(crash-reporter) leave_core" \
    "${rootfs}" \
    /root/.leave_core \
    "${FLAGS_leave_core}"

  process_tag "${do_modification}" \
    "(crosh) crosh_workarounds" \
    "${rootfs}" \
    /usr/bin/crosh-workarounds \
    "${FLAGS_crosh_workarounds}"
}

# Iterates through all options for manipulating the lsb-release.
# Args: ROOTFS DO_MODIFICATION
process_all_lsb_mods() {
  local rootfs="$1"
  local do_modifications="$2"
  local lsb="${rootfs}/etc/lsb-release"
  local sudo

  if [ ! -w "${lsb}" ]; then
    sudo="sudo"
  fi

  if [ ${FLAGS_remove_test_label} = ${FLAGS_TRUE} ]; then
    if grep -wq "test" "${lsb}"; then
      g_modified=${FLAGS_TRUE}
    fi
    if [ ${do_modifications} = ${FLAGS_TRUE} ]; then
      ${sudo} sed -i 's/\btest\b//' "${lsb}" &&
        echo "Test Label removed from /etc/lsb-release"
    fi
  fi

  if [ ${FLAGS_change_dev_to_beta} = ${FLAGS_TRUE} ]; then
    if grep -wq "dev" "${lsb}"; then
      g_modified=${FLAGS_TRUE}
    fi
    if [ ${do_modifications} = ${FLAGS_TRUE} ]; then
      ${sudo} sed -i 's/\bdev\b/beta/' "${lsb}" &&
        echo "Dev Channel Label was changed to Beta"
    fi
  fi
}


IMAGE=$(readlink -f "${FLAGS_from}")
if [[ -z "${IMAGE}" || ! -f "${IMAGE}" ]]; then
  echo "Missing required argument: --from (image to update)"
  usage
  exit 1
fi

# First round, mount as read-only and check if we need any modifications.
rootfs=$(make_temp_dir)
mount_image_partition_ro "${IMAGE}" 3 "${rootfs}"

# we don't have tags in stateful partition yet...
# stateful_dir=$(make_temp_dir)
# mount_image_partition ${IMAGE} 1 ${stateful_dir}

process_all_tags "${rootfs}" ${FLAGS_FALSE}
process_all_lsb_mods "${rootfs}" ${FLAGS_FALSE}

if [ ${g_modified} = ${FLAGS_TRUE} ]; then
  # remount as RW (we can't use mount -o rw,remount because of loop device)
  sudo umount "${rootfs}"
  mount_image_partition "${IMAGE}" 3 "${rootfs}"

  # second round, apply the modification to image.
  process_all_tags "${rootfs}" ${FLAGS_TRUE}
  process_all_lsb_mods "${rootfs}" ${FLAGS_TRUE}

  # this is supposed to be automatically done in mount_image_partition,
  # but it's no harm to explicitly make it again here.
  tag_as_needs_to_be_resigned "${rootfs}"
  echo "IMAGE IS MODIFIED. PLEASE REMEMBER TO RESIGN YOUR IMAGE."
else
  echo "Image is not modified."
fi
