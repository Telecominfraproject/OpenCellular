#!/bin/sh

# this is a common helper script that is shared among all BTS model
# specific helper scripts like jenkins_sysmobts.sh.  You shouldn't call
# this directly, but rather indirectly via the bts-specific scripts

if ! [ -x "$(command -v osmo-deps.sh)" ]; then
	echo "Error: We need to have scripts/osmo-deps.sh from http://git.osmocom.org/osmo-ci/ in PATH !"
	exit 2
fi

set -ex

base="$PWD"
deps="$base/deps"
inst="$deps/install"

export deps inst

osmo-clean-workspace.sh

mkdir -p "$deps"

verify_value_string_arrays_are_terminated.py $(find . -name "*.[hc]")

# generic project build function, usage:
# build "PROJECT-NAME" "CONFIGURE OPTIONS"
build_bts() {
    set +x
    echo
    echo
    echo
    echo " =============================== $1 ==============================="
    echo
    set -x

    cd $deps
    osmo-deps.sh libosmocore
    cd $base
    shift
    conf_flags="$*"
    autoreconf --install --force
    ./configure $conf_flags
    $MAKE $PARALLEL_MAKE
    $MAKE check || cat-testlogs.sh
    DISTCHECK_CONFIGURE_FLAGS="$conf_flags" $MAKE distcheck || cat-testlogs.sh
}
