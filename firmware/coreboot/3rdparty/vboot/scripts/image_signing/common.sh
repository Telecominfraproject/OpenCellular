#!/bin/bash
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

. "$(dirname "$0")/common_minimal.sh"
CROS_LOG_PREFIX="${PROG}: "

# Array of actions that are executed during the clean up process.
declare -a cleanup_actions

# Adds an action to be executed during the clean up process.
# Actions are executed in the reverse order of when they were added.
# ARGS: ACTION
add_cleanup_action() {
  cleanup_actions[${#cleanup_actions[*]}]=$1
}

# Performs the latest clean up action and removes it from the list.
perform_latest_cleanup_action() {
  local num_actions=${#cleanup_actions[*]}
  if [ ${num_actions} -gt 0 ]; then
    eval "${cleanup_actions[$num_actions-1]}" > /dev/null 2>&1
    unset cleanup_actions[$num_actions-1]
  fi
}

# Performs clean up by executing actions in the cleanup_actions array in
# reversed order.
cleanup() {
  set +e

  while [ ${#cleanup_actions[*]} -gt 0 ]; do
    perform_latest_cleanup_action
  done

  set -e
}

# ANSI color codes used when displaying messages.
# Taken from src/scripts/common.sh.
V_RED="\e[31m"
V_YELLOW="\e[33m"
V_BOLD_GREEN="\e[1;32m"
V_BOLD_RED="\e[1;31m"
V_BOLD_YELLOW="\e[1;33m"
V_VIDOFF="\e[0m"

# Prints an informational message.
# Taken from src/scripts/common.sh.
# Arg: MESSAGE
info() {
  echo -e >&2 "${V_BOLD_GREEN}${CROS_LOG_PREFIX:-}INFO   : $*${V_VIDOFF}"
}

# Prints a warning message.
# Taken from src/scripts/common.sh.
# Arg: MESSAGE
warn() {
  echo -e >&2 "${V_BOLD_YELLOW}${CROS_LOG_PREFIX:-}WARNING: $*${V_VIDOFF}"
}

# Prints the specified error and exit the script with an error code.
# Taken from src/scripts/common.sh.
# Args: MESSAGE
error() {
  echo -e >&2   "${V_BOLD_RED}${CROS_LOG_PREFIX:-}ERROR  : $*${V_VIDOFF}"
}

TEMP_LOOP_LIST=$(mktemp)

# Setup a loopback device for a file and scan for partitions, with retries.
#
# $1 - The file to back the new loopback device.
# $2-$N - Additional arguments to pass to losetup.
loopback_partscan() {
  local lb_dev image="$1"
  shift

  # We know partition scanning & dev node creation can be racy with udev and
  # the kernel, and the kernel does not sleep/wait for it to finish.  We have
  # to use the partx tool manually as it will sleep until things are finished.
  lb_dev=$(sudo losetup --show -f "$@" "${image}")

  # Cache the path so we can clean it up.
  echo "${lb_dev}" >>"${TEMP_LOOP_LIST}"

  # Ignore problems deleting existing partitions. There shouldn't be any
  # which will upset partx, but that's actually ok.
  sudo partx -d "${lb_dev}" 2>/dev/null || true
  sudo partx -a "${lb_dev}"

  echo "${lb_dev}"
}

# Detach a loopback device set up earlier.
#
# $1 - The loop device to detach.
# $2-$N - Additional arguments to pass to losetup.
loopback_detach() {
  # Retry the deletes before we detach.  crbug.com/469259
  local i
  for (( i = 0; i < 10; i++ )); do
    if sudo partx -d "$1"; then
      break
    fi
    warn "Sleeping & retrying ..."
    sync
    sleep 1
  done
  sudo losetup --detach "$@"
}

# Clear out all loopback devices we setup.
cleanup_loopbacks() {
  local line
  while read -r line; do
    loopback_detach "${line}" 2>/dev/null
  done <"${TEMP_LOOP_LIST}"
  rm -f "${TEMP_LOOP_LIST}"
}

# Usage: lsbval path-to-lsb-file key
# Returns the value for the given lsb-release file variable.
lsbval() {
  local lsbfile="$1"
  local key="$2"
  grep "^${key}=" "${lsbfile}" | sed "s/^${key}=//"
}

# Usage: get_board_from_lsb_release rootfs
# Returns the exact board name from /etc/lsb-release.  This may contain
# dashes or other characters not suitable for variable names.  See the
# next function for that.
get_board_from_lsb_release() {
  local rootfs="$1"
  lsbval "${rootfs}/etc/lsb-release" CHROMEOS_RELEASE_BOARD
}

# Usage: get_boardvar_from_lsb_release rootfs
# Returns the board name from /etc/lsb-release in a mangled form that can
# be used in variable names.  e.g. dashes are turned into underscores.
get_boardvar_from_lsb_release() {
  get_board_from_lsb_release "$@" | sed 's:[-]:_:g'
}

# This will override the trap set in common_minmal.sh
trap "cleanup" INT TERM EXIT

add_cleanup_action "cleanup_temps_and_mounts"
add_cleanup_action "cleanup_loopbacks"
