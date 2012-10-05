#!/bin/bash

# Copyright (c) 2012 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Abort on error.
set -e

LSB_FILE=/etc/lsb-release

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

usage() {
  echo "Usage $PROG image [config]"
}

# Usage: lsbval path-to-lsb-file key
# Returns the value for the given lsb-release file variable.
lsbval() {
  local lsbfile="$1"
  local key="$2"
  grep ^$key= "$lsbfile" | sed s/^$key=//
}

# Usage: lsbequals path-to-lsb-file key expected-value
# Returns 0 if they match, 1 otherwise.
# Also outputs a warning message if they don't match.
lsbequals() {
  local lsbfile="$1"
  local key="$2"
  local expectval="$3"
  local realval=$(lsbval "$lsbfile" $key)
  if [ "$realval" != "$expectval" ]; then
    echo "$key mismatch. Expected '$expectval', image contains '$realval'"
    return 1
  fi
  return 0
}

# Usage: check_keyval_in_list lsbfile lsbkey [list of values]
# Extracts the lsb-release value for the specified key, and confirms it
# matches one of the whitelisted values specified in value_array.
# Implementation note:
# You can't really pass bash arrays to functions. Best you can do is either
# serialize to string/pass/deserialize (e.g. using whitspace/IFS hacks), or,
# let the array contents be received as multiple arguments to the target
# function. We take the latter approach here, hence the shift's to get the
# first 2 arguments out, before we process the rest of the varargs.
check_keyval_in_list() {
  local lsbfile="$1"
  shift
  local lsbkey="$1"
  shift
  local lsbval=$(lsbval "$lsbfile" "$lsbkey")
  while [ $# -gt 0 ]; do
    if [ "$lsbval" == "$1" ]; then
      return 0
    fi
    shift
  done
  # If we get here, it wasn't found
  echo "$lsbkey: Value '$lsbval' was not recognized"
  return 1
}

# Usage: lsb_syntaxcheck path-to-lsb-file
# Enforces a number of basic sanity checks on the overall format and contents
# of the lsb-release file:
# - Every line is "key=value".
# - No space after key, no space before value.
# - key is all A-Z or _, but not starting with _.
# - value is made up of printable characters, or is empty.
# - Each line is a reasonable size (<255 bytes).
# - The whole file is a reasonable size (4kb).
lsb_syntaxcheck() {
  local lsbfile="$1"
  syntaxbad=0
  # Checks for key being A-Z_, 1 or more characters, not starting with _.
  # Also checks for = with no spaces on either side.
  # Checks that the value contains printables (and not starting with space).
  # Alternatively, the value is permitted to be empty (0 chars) too.
  badlines=$(grep -Ev '^[A-Z][A-Z_]*=([[:graph:]][[:print:]]*)?$' "$lsbfile")
  if [ -n "$badlines" ]; then
    syntaxbad=1
    echo "$lsbfile: Some lines seem non-well-formed:"
    echo "$badlines"
  fi

  # Checks for a lines exceeding a reasonable overall length.
  badlines=$(grep -E '^.{255}' "$lsbfile")
  if [ -n "$badlines" ]; then
    syntaxbad=1
    echo "$lsbfile: Some lsb-release lines seem unreasonably long:"
    echo "$badlines"
  fi
  # Overall file size check:
  size=$(ls -sk "$lsbfile" | cut -d ' ' -f 1)
  if [ $size -gt 4 ]; then
    syntaxbad=1
    echo "$lsbfile: This file exceeds 4kb"
  fi
  return $syntaxbad
}

main() {
  # We want to catch all the discrepancies, not just the first one.
  # So, any time we find one, we set testfail=1 and continue.
  # When finished we will use testfail to determine our exit value.
  local testfail=0

  if [ $# -ne 1 ] && [ $# -ne 2 ]; then
    usage
    exit 1
  fi

  local image="$1"

  # Default config location: same directory as this script.
  local configfile="$(dirname "$0")/default_lsb_release.config"
  # Or, maybe a config was provided on the command line.
  if [ $# -eq 2 ]; then
    configfile="$2"
  fi
  # Either way, load test-expectations data from config.
  echo -n "Loading config from $configfile... "
  . "$configfile" || return 1
  echo "Done."

  local rootfs=$(make_temp_dir)
  mount_image_partition_ro "$image" 3 "$rootfs"
  local lsb="$rootfs/$LSB_FILE"

  # Basic syntax check first.
  lsb_syntaxcheck "$lsb" || testfail=1

  lsbequals $lsb CHROMEOS_AUSERVER "$expected_auserver" || testfail=1
  lsbequals $lsb CHROMEOS_RELEASE_NAME "$expected_release_name" || testfail=1
  check_keyval_in_list $lsb CHROMEOS_RELEASE_TRACK \
    "${expected_release_tracks[@]}" || testfail=1

  if check_keyval_in_list $lsb CHROMEOS_RELEASE_BOARD \
    "${expected_boards[@]}"; then
    # Pick the right set of test-expectation data to use. The cuts
    # turn e.g. x86-foo-pvtkeys into x86-foo.
    local board=$(lsbval $lsb CHROMEOS_RELEASE_BOARD |
                  cut -d = -f 2 |
                  cut -d - -f 1,2)
    # a copy of the board string with '-' squished to variable-name-safe '_'.
    local boardvar=${board//-/_}
    channel=$(lsbval $lsb CHROMEOS_RELEASE_TRACK)
    # For a canary or dogfood channel, appid maybe a different default value.
    if [ $channel = 'canary-channel' ] || [ $channel = 'dogfood-channel' ]; then
      eval "expected_appid=\"\$expected_appid_${channel%\-channel}\""
    else
      eval "expected_appid=\"\$expected_appid_$boardvar\""
    fi
    lsbequals $lsb CHROMEOS_RELEASE_APPID "$expected_appid" || testfail=1
  else # unrecognized board
    testfail=1
  fi

  exit $testfail
}

main "$@"
