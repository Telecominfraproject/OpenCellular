DESCRIPTION = "Set an early date on RTC less systems"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "\
	file://early-date \
	file://early-date.service"

FILES_${PN} = "${systemd_system_unitdir} /sbin"
RDPEPENDS_${PN} = "systemd"

do_install() {
	install -d ${D}/sbin
	install -d ${D}${systemd_system_unitdir}/basic.target.wants

	# Copy the service file and link it
	install -m 0644 ${WORKDIR}/early-date.service ${D}${systemd_system_unitdir}
	ln -sf ../early-date.service ${D}${systemd_system_unitdir}/basic.target.wants/

	# Hardcode to /sbin
	# TODO: Set the date as of the build time..
	install -m 0755 ${WORKDIR}/early-date ${D}/sbin
}
