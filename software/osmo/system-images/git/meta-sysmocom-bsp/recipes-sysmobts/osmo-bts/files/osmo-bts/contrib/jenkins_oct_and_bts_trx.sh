#!/bin/sh
# jenkins build helper script for osmo-bts-octphy + osmo-bts-trx

# shellcheck source=contrib/jenkins_common.sh
. $(dirname "$0")/jenkins_common.sh

export PKG_CONFIG_PATH="$inst/lib/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="$inst/lib"

osmo-build-dep.sh libosmocore "" --disable-doxygen

osmo-build-dep.sh libosmo-abis

cd "$deps"

osmo-layer1-headers.sh oct "$FIRMWARE_VERSION"

configure_flags="\
  --with-osmo-pcu=$deps/osmo-pcu/include \
  --with-octsdr-2g=$deps/layer1-headers/ \
  --enable-octphy \
  --enable-trx \
  "

build_bts "osmo-bts-octphy+trx" "$configure_flags"

osmo-clean-workspace.sh
