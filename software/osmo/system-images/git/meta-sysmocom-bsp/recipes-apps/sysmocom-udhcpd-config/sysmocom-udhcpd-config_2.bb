DESCRIPTION = "Task for sysmocom external tools"
LICENSE = "MIT"
LIC_FILES_CHKSUM = " \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "file://udhcpd.conf \
           file://udhcpd.service \
          "
PR = "r0"

CONFFILES_${PN} = "${sysconfdir}/udhcpd.conf"
FILES_${PN} += "${systemd_unitdir}"

do_install() {
	install -d ${D}${sysconfdir}
	install -m 0644 ${WORKDIR}/udhcpd.conf ${D}${sysconfdir}/
        install -d ${D}${systemd_system_unitdir}/multi-user.target.wants/
        install -m 0644 /${WORKDIR}/udhcpd.service ${D}${systemd_system_unitdir}/
        ln -sf ../udhcpd.service ${D}${systemd_system_unitdir}/multi-user.target.wants/
}
