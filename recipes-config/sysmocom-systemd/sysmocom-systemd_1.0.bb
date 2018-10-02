DESCRIPTION = "sysmocom systemd customization"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = " \
	file://emergency.service \
"

do_install() {
	install -d ${D}${sysconfdir}/systemd/system
	install -m 0644 ${WORKDIR}/emergency.service ${D}${sysconfdir}/systemd/system
}

CONFFILES_${PN} += "${sysconfdir}/systemd/system/emergency.service"
