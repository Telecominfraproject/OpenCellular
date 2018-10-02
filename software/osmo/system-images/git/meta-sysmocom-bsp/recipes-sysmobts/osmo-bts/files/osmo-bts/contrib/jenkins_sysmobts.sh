#!/bin/sh
# jenkins build helper script for osmo-bts-sysmo

# shellcheck source=contrib/jenkins_common.sh
. $(dirname "$0")/jenkins_common.sh

osmo-build-dep.sh libosmocore "" --disable-doxygen

export PKG_CONFIG_PATH="$inst/lib/pkgconfig:$PKG_CONFIG_PATH"
export LD_LIBRARY_PATH="$inst/lib"

osmo-build-dep.sh libosmo-abis

cd "$deps"
osmo-layer1-headers.sh sysmo "$FIRMWARE_VERSION"
mkdir -p "$inst/include/sysmocom/femtobts"
ln -s $deps/layer1-headers/include/* "$inst/include/sysmocom/femtobts/"

configure_flags="--enable-sanitize --enable-sysmocom-bts --with-sysmobts=$inst/include/"

# This will not work for the femtobts
if [ $FIRMWARE_VERSION != "femtobts_v2.7" ]; then
    configure_flags="$configure_flags --enable-sysmobts-calib"
fi

build_bts "osmo-bts-sysmo" "$configure_flags"

osmo-clean-workspace.sh
