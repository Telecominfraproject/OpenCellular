# Depend on the osmocom toolchain
require recipes-osmocom/meta/meta-toolchain-osmo.bb

# Change the name
TOOLCHAIN_OUTPUTNAME = "${SDK_NAME}-toolchain-sysmobts-${DISTRO_VERSION}-${DATETIME}"

# Add API headers..
TOOLCHAIN_TARGET_TASK += "femtobts-api-dev"

