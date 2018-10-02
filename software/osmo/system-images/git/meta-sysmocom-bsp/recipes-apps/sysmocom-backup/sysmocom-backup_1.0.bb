DESCRIPTION = "sysmocom config backup and restore scripts"
LICENSE = "GPLv3+"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
PR = "r15"

SRC_URI = " \
	file://sysmocom-backup \
	file://sysmocom-restore \
	file://default.files \
"
RDEPENDS_${PN} = "tar"

do_install() {
	install -d ${D}${sbindir}
	install -m 0755 ${WORKDIR}/sysmocom-backup  ${D}${sbindir}/
	install -m 0755 ${WORKDIR}/sysmocom-restore ${D}${sbindir}/

	install -d ${D}${sysconfdir}/sysmocom/backup.d
	install -m 0644 ${WORKDIR}/default.files ${D}${sysconfdir}/sysmocom/backup.d/
}

PACKAGES =+ "${PN}-default"

FILES_${PN}-default = "${sysconfdir}/sysmocom/backup.d/"
