#!/bin/bash
# Copyright (c) 2013 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Color output encodings.
COL_RED='\E[31;1m'
COL_GREEN='\E[32;1m'
COL_YELLOW='\E[33;1m'
COL_BLUE='\E[34;1m'
COL_STOP='\E[0;m'

# args: [message]
green() {
  echo -e "${COL_GREEN}$*${COL_STOP}"
}

# args: [message]
yellow() {
  echo -e "${COL_YELLOW}WARNING: $*${COL_STOP}"
}

# args: [message]
red() {
  echo -e "${COL_RED}$*${COL_STOP}"
}

# args: [nested level] [message]
error() {
  local lev=${1:-}
  case "${1:-}" in
    [0-9]*)
      lev=$1
      shift
      ;;
    *) lev=0
      ;;
  esac
  local x=$(caller $lev)
  local cline="${x%% *}"
  local cfile="${x#* }"
  cfile="${cfile##*/}"
  local args="$*"
  local spacer="${args:+: }"
  red "at ${cfile}, line ${cline}${spacer}${args}" 1>&2
  exit 1
}
