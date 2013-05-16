#!/bin/bash

# Copyright (c) 2011 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

# Abort on error.
set -e

# Load common constants and variables.
. "$(dirname "$0")/common.sh"

# Given a kernel boot param string which includes ...dm="dmstuff"...
# this returns the dmstuff by itself.
get_dmparams() {
    echo "$1" | sed 's/^.*\ dm="\([^"]*\)".*/\1/'
}

# Given a kernel boot param string which includes ...dm="stuff"...
# this returns the param string with the dm="..." section removed.
# Useful in conjunction with get_dmparams to divide and process
# the two sections of parameters in seperate passes
kparams_remove_dm() {
    echo "$1" | sed 's/dm="[^"]*"//'
}

# Given a dm param string which includes dynamic values, return the
# same string with these values replaced by a magic string placeholder.
# This same magic placeholder is used in the config file, for comparison
# purposes.
dmparams_mangle() {
  local dmparams=$1
  # First handle new key-value style verity parameters.
  dmparams=$(echo "$dmparams" |
    sed -e 's/root_hexdigest=[0-9a-fA-F]*/root_hexdigest=MAGIC_HASH/' |
    sed -e 's/salt=[0-9a-fA-F]*/salt=MAGIC_SALT'/)
  # If we didn't substitute the MAGIC_HASH yet, these are the old
  # verity parameter format.
  if [[ $dmparams != *MAGIC_HASH* ]]; then
    dmparams=$(echo $dmparams | sed 's/sha1 [0-9a-fA-F]*/sha1 MAGIC_HASH/')
  fi
  # If we have bootcache enabled, replace its copy of the root_hexdigest
  # with MAGIC_HASH. The parameter is positional.
  if [[ $dmparams == *bootcache* ]]; then
    dmparams=$(echo $dmparams |
      sed -r 's:(bootcache (PARTUUID=)?%U(/PARTNROFF=|\+)1 [0-9]+) [0-9a-fA-F]+:\1 MAGIC_HASH:')
  fi
  echo $dmparams
}

# This escapes any non-alphanum character, since many such characters
# are regex metacharacters.
escape_regexmetas() {
    echo "$1" | sed 's/\([^a-zA-Z0-9]\)/\\\1/g'
}

usage() {
    echo "Usage $PROG image [config]"
}

main() {
    # We want to catch all the discrepancies, not just the first one.
    # So, any time we find one, we set testfail=1 and continue.
    # When finished we will use testfail to determine our exit value.
    local testfail=0
    # A buffer to include useful information that we dump when things fail.
    local output
    # Copy of a string before it has been through sed
    local pre_sed

    if [[ $# -ne 1 ]] && [[ $# -ne 2 ]]; then
        usage
        exit 1
    fi

    local image="$1"

    # A byte that should not appear in the command line to use as a sed
    # marker when doing regular expression replacements.
    local M=$'\001'

    # Default config location: same name/directory as this script,
    # with a .config file extension, ie ensure_secure_kernelparams.config.
    local configfile="$(dirname "$0")/${0/%.sh/.config}"
    # Or, maybe a config was provided on the command line.
    if [[ $# -eq 2 ]]; then
        configfile="$2"
    fi
    # Either way, load test-expectations data from config.
    . "$configfile" || return 1

    local kernelblob=$(make_temp_file)
    # TODO(jimhebert): Perform the kernel security tests on both the kernel
    #                  partitions. Here, we just run it on kernel partition 4
    #                  which is the install kernel on the recovery image.
    #                  crosbug.com/24274
    extract_image_partition "$image" 4 "$kernelblob"
    local rootfs=$(make_temp_dir)
    mount_image_partition_ro "$image" 3 "$rootfs"

    # Pick the right set of test-expectation data to use. The cuts
    # turn e.g. x86-foo as a well as x86-foo-pvtkeys into x86_foo.
    local board=$(grep CHROMEOS_RELEASE_BOARD= "$rootfs/etc/lsb-release" | \
                  cut -d = -f 2 | cut -d - -f 1,2 --output-delimiter=_)
    eval "required_kparams=(\"\${required_kparams_$board[@]}\")"
    eval "required_kparams_regex=(\"\${required_kparams_regex_$board[@]}\")"
    eval "optional_kparams=(\"\${optional_kparams_$board[@]}\")"
    eval "optional_kparams_regex=(\"\${optional_kparams_regex_$board[@]}\")"
    eval "required_dmparams=(\"\${required_dmparams_$board[@]}\")"
    eval "required_dmparams_regex=(\"\${required_dmparams_regex_$board[@]}\")"
    output+="required_kparams=(\n"
    output+="$(printf "\t'%s'\n" "${required_kparams[@]}")\n)\n"
    output+="required_kparams_regex=(\n"
    output+="$(printf "\t'%s'\n" "${required_kparams_regex[@]}")\n)\n"
    output+="optional_kparams=(\n"
    output+="$(printf "\t'%s'\n" "${optional_kparams[@]}")\n)\n"
    output+="optional_kparams_regex=(\n"
    output+="$(printf "\t'%s'\n" "${optional_kparams_regex[@]}")\n)\n"
    output+="required_dmparams=(\n"
    output+="$(printf "\t'%s'\n" "${required_dmparams[@]}")\n)\n"
    output+="required_dmparams_regex=(\n"
    output+="$(printf "\t'%s'\n" "${required_dmparams_regex[@]}")\n)\n"

    # Divide the dm params from the rest and process seperately.
    local kparams=$(dump_kernel_config "$kernelblob")
    local dmparams=$(get_dmparams "$kparams")
    local kparams_nodm=$(kparams_remove_dm "$kparams")

    output+="\nkparams='${kparams}'\n"
    output+="\ndmparams='${dmparams}'\n"
    output+="\nkparams_nodm='${kparams_nodm}'\n"

    mangled_dmparams=$(dmparams_mangle "${dmparams}")
    output+="\nmangled_dmparams='${mangled_dmparams}'\n"
    # Special-case handling of the dm= param:
    testfail=1
    for expected_dmparams in "${required_dmparams[@]}"; do
      # Filter out all dynamic parameters.
      if [ "$mangled_dmparams" = "$expected_dmparams" ]; then
        testfail=0
        break
      fi
    done

    for expected_dmparams in "${required_dmparams_regex[@]}"; do
      if [[ -z $(echo "${mangled_dmparams}" | \
           sed "s${M}^${expected_dmparams}\$${M}${M}") ]]; then
        testfail=0
        break
      fi
    done

    if [ $testfail -eq 1 ]; then
        echo "Kernel dm= parameter does not match any expected values!"
        echo "Actual:   $dmparams"
        echo "Expected: ${required_dmparams[@]}"
    fi

    # Ensure all other required params are present.
    for param in "${required_kparams[@]}"; do
        if [[ "$kparams_nodm" != *$param* ]]; then
            echo "Kernel parameters missing required value: $param"
            testfail=1
        else
            # Remove matched params as we go. If all goes well, kparams_nodm
            # will be nothing left but whitespace by the end.
            param=$(escape_regexmetas "$param")
            kparams_nodm=$(echo " ${kparams_nodm} " |
                           sed "s${M} ${param} ${M} ${M}")
        fi
    done

    # Ensure all other required regex params are present.
    for param in "${required_kparams_regex[@]}"; do
      pre_sed=" ${kparams_nodm} "
      kparams_nodm=$(echo "${pre_sed}" | sed "s${M} ${param} ${M} ${M}")
      if [[ "${pre_sed}" == "${kparams_nodm}" ]]; then
        echo "Kernel parameters missing required value: ${param}"
        testfail=1
      fi
    done

    # Check-off each of the allowed-but-optional params that were present.
    for param in "${optional_kparams[@]}"; do
        param=$(escape_regexmetas "$param")
        kparams_nodm=$(echo " ${kparams_nodm} " |
                       sed "s${M} ${param} ${M} ${M}")
    done

    # Check-off each of the allowed-but-optional params that were present.
    for param in "${optional_kparams_regex[@]}"; do
        kparams_nodm=$(echo " ${kparams_nodm} " |
                       sed "s${M} ${param} ${M} ${M}")
    done

    # This section enforces the default-deny for any unexpected params
    # not already processed by one of the above loops.
    if [[ ! -z ${kparams_nodm// /} ]]; then
        echo "Unexpected kernel parameters found:"
        echo " $(echo "${kparams_nodm}" | sed -r 's:  +: :g')"
        testfail=1
    fi

    if [[ ${testfail} -eq 1 ]]; then
        echo "Debug output:"
        printf '%b\n' "${output}"
        echo "(actual error will be at the top of output)"
    fi

    exit $testfail
}

main $@
