DESCRIPTION = "sysmocom config backup and restore scripts"
LICENSE = "GPLv3+"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
PR = "r13"

SRC_URI = " \
	file://sysmocom-backup-data \
	file://sysmocom-restore-data \
	file://data.mount \
	file://sysmocom-restore.service \
"
RDEPENDS_${PN} = "tar"
RCONFLICTS_${PN} = "symocom-backup"

inherit systemd

do_install() {
	install -d ${D}${sbindir}
	install -m 0755 ${WORKDIR}/sysmocom-backup-data ${D}${sbindir}/
	install -m 0755 ${WORKDIR}/sysmocom-restore-data ${D}${sbindir}/

	install -d ${D}${systemd_system_unitdir}
	install -m 0644 ${WORKDIR}/data.mount ${D}${systemd_system_unitdir}/
	install -m 0644 ${WORKDIR}/sysmocom-restore.service ${D}${systemd_system_unitdir}/
}

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "data.mount sysmocom-restore.service"
SYSTEMD_AUTO_ENABLE_${PN} = "disable"
