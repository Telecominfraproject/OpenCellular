DEFAULTTUNE ?= "geode"

require conf/machine/include/tune-i586.inc

# Extra tune features
TUNEVALID[geode] = "Enable geode specific processor optimizations"
TUNE_CCARGS += "${@bb.utils.contains('TUNE_FEATURES', 'geode', '-march=geode -mtune=geode', '', d)}"

# Extra tune selections
AVAILTUNES += "geode"
TUNE_FEATURES_tune-geode ?= "${TUNE_FEATURES_tune-x86} geode"
BASE_LIB_tune-geode ?= "lib"
TUNE_PKGARCH_tune-geode = "geode"
PACKAGE_EXTRA_ARCHS_tune-geode = "${PACKAGE_EXTRA_ARCHS_tune-x86} i386 i486 i586 geode"
