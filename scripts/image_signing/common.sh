#!/bin/bash
#
# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

. "$(dirname "$0")/common_minimal.sh"

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
  echo -e >&2 "${V_BOLD_GREEN}INFO   : $1${V_VIDOFF}"
}

# Prints a warning message.
# Taken from src/scripts/common.sh.
# Arg: MESSAGE
warn() {
  echo -e >&2 "${V_BOLD_YELLOW}WARNING: $1${V_VIDOFF}"
}

# Prints the specified error and exit the script with an error code.
# Taken from src/scripts/common.sh.
# Args: MESSAGE
error() {
  echo -e >&2   "${V_BOLD_RED}ERROR  : $1${V_VIDOFF}"
}

# Prints an error message and exit with an error code.
# Taken from src/scripts/common.sh.
# Args: MESSAGE
die() {
  error "$1"
  exit 1
}

# This will override the trap set in common_minmal.sh
trap "cleanup" INT TERM EXIT

add_cleanup_action "cleanup_temps_and_mounts"
