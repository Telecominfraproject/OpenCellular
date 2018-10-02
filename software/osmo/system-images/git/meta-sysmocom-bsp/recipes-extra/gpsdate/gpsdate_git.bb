DESCRIPTION = "A utility to set system RTC to GPSD time"
SECTION = "console/network"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"
DEPENDS = "gpsd"
RDEPENDS_${PN} = "libgps"

PE = "1"
PR = "r14"
PV = "0.2+git${SRCPV}"

SRC_URI = "git://git.sysmocom.de/gpsdate.git;branch=master \
	file://gpsdate.default \
"
SRCREV = "cd7b77ef311f317aac7a067308a94e46811a20f2"
S = "${WORKDIR}/git"

INITSCRIPT_NAME = "gpsdate"
INITSCRIPT_PARAMS = "defaults 35"

inherit update-rc.d systemd

SYSTEMD_SERVICE_${PN} = "${PN}.service"

do_install() {
    install -d ${D}/${sbindir}
    install -m 0755 ${S}/gpsdate ${D}/${sbindir}/gpsdate
    install -m 0755 ${S}/gps-watchdog ${D}/${sbindir}/gps-watchdog
    install -d ${D}/${sysconfdir}/init.d
    install -m 0755 ${S}/gpsdate.init ${D}/${sysconfdir}/init.d/gpsdate

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${S}/gpsdate.service ${D}${systemd_system_unitdir}

    install -d ${D}/${sysconfdir}/default
    install -m 0644 ${WORKDIR}/gpsdate.default ${D}/${sysconfdir}/default/gpsdate
}

PACKAGES =+ "gps-watchdog"

FILES_gps-watchdog = "${sbindir}/gps-watchdog"
