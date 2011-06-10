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

# Given a dm param string which includes a long and unpredictable
# sha1 hash, return the same string with the sha1 hash replaced
# with a magic placeholder. This same magic placeholder is used
# in the config file, for comparison purposes.
dmparams_mangle_sha1() {
    echo "$1" | sed 's/sha1 [0-9a-fA-F]*/sha1 MAGIC_HASH/'
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

    if [[ $# -ne 1 ]] && [[ $# -ne 2 ]]; then
        usage
        exit 1
    fi

    local image="$1"

    # Default config location: same name/directory as this script,
    # with a .config file extension, ie ensure_secure_kernelparams.config.
    local configfile="$(dirname "$0")/${0/%.sh/.config}"
    # Or, maybe a config was provided on the command line.
    if [[ $# -eq 2 ]]; then
        configfile="$2"
    fi
    # Either way, load test-expectations data from config.
    . "$configfile"

    local kernelblob=$(make_temp_file)
    extract_image_partition "$image" 2 "$kernelblob"
    local rootfs=$(make_temp_dir)
    mount_image_partition_ro "$image" 3 "$rootfs"

    # Pick the right set of test-expectation data to use. The cuts
    # turn e.g. x86-foo as a well as x86-foo-pvtkeys into x86_foo.
    local board=$(grep CHROMEOS_RELEASE_BOARD= "$rootfs/etc/lsb-release" | \
                  cut -d = -f 2 | cut -d - -f 1,2 --output-delimiter=_)
    eval "required_kparams=(\${required_kparams_$board[@]})"
    eval "optional_kparams=(\${optional_kparams_$board[@]})"
    eval "optional_kparams_regex=(\${optional_kparams_regex_$board[@]})"
    eval "required_dmparams=\"\$required_dmparams_$board\""

    # Divide the dm params from the rest and process seperately.
    local kparams=$(dump_kernel_config "$kernelblob")
    local dmparams=$(dmparams_mangle_sha1 "$(get_dmparams "$kparams")")
    local kparams_nodm=$(kparams_remove_dm "$kparams")

    # Special-case handling of the dm= param:
    if [[ "$dmparams" != "$required_dmparams" ]]; then
        echo "Kernel dm= parameter does not match expected value!"
        echo "Expected: $required_dmparams"
        echo "Actual:   $dmparams"
        testfail=1
    fi

    # Ensure all other required params are present.
    for param in ${required_kparams[@]}; do
        if [[ "$kparams_nodm" != *$param* ]]; then
            echo "Kernel parameters missing required value: $param"
            testfail=1
        else
            # Remove matched params as we go. If all goes well, kparams_nodm
            # will be nothing left but whitespace by the end.
            param=$(escape_regexmetas "$param")
            kparams_nodm=$(echo "$kparams_nodm" | sed "s/\b$param\b//")
        fi
    done

    # Check-off each of the allowed-but-optional params that were present.
    for param in ${optional_kparams[@]}; do
        param=$(escape_regexmetas "$param")
        kparams_nodm=$(echo "$kparams_nodm" | sed "s/\b$param\b//")
    done

    # Check-off each of the allowed-but-optional params that were present.
    for param in ${optional_kparams_regex[@]}; do
        kparams_nodm=$(echo "$kparams_nodm" | sed "s/\b$param\b//")
    done

    # This section enforces the default-deny for any unexpected params
    # not already processed by one of the above loops.
    if [[ ! -z ${kparams_nodm// /} ]]; then
        echo "Unexpected kernel parameters found: $kparams_nodm"
        testfail=1
    fi

    exit $testfail
}

main $@
