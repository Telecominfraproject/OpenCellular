DESCRIPTION = "Task for sysmoBTS"
LICENSE = "MIT"
LIC_FILES_CHKSUM = " \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
DEPENDS = "virtual/kernel"
ALLOW_EMPTY_${PN} = "1"
PR = "r25"

CALIB = ""
CALIB_sysmobts-v2 = "sysmobts-calib sysmobts-util"

UTIL = ""
UTIL_sysmobts-v2 = "sbts2050-util gpsd gps-utils gpsdate"
UTIL_sysmobts2100 = "gpsd gps-utils gpsdate lc15-sysdev-remap"
UTIL_oc2g = "gpsd gps-utils gpsdate oc2g-sysdev-remap"

# TODO: re-add femtobts-calib after it went through the API migration
RDEPENDS_${PN} = "\
    osmo-bts \
    osmo-pcu \
    lmsensors-scripts \
    ${CALIB} \
    ${UTIL} \
    "
RDEPENDS_${PN}_append_sysmobts-v2 = " osmo-bts-remote sysmobts-config"
PACKAGE_ARCH = "${MACHINE_ARCH}"
