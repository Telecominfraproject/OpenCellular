#!/bin/sh
# jenkins build helper script for osmo-bts-lc15

# shellcheck source=contrib/jenkins_common.sh
. $(dirname "$0")/jenkins_common.sh

osmo-build-dep.sh libosmocore "" --disable-doxygen

export PKG_CONFIG_PATH="$inst/lib/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="$inst/lib"

osmo-build-dep.sh libosmo-abis

cd "$deps"
osmo-layer1-headers.sh lc15 "$FIRMWARE_VERSION"

configure_flags="--enable-sanitize --with-litecell15=$deps/layer1-headers/inc/ --enable-litecell15"

build_bts "osmo-bts-lc15" "$configure_flags"

osmo-clean-workspace.sh
