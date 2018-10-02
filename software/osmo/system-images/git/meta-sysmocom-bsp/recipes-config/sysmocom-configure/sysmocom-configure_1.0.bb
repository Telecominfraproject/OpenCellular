DESCRIPTION = "sysmocom configuration scripts"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = " \
	file://sysmocom-generator \
	file://multi-user.target.sh \
	file://osmo-bsc.service.sh \
"

do_install() {
	install -d ${D}${systemd_unitdir}/system-generators
	install -m 0755 ${WORKDIR}/sysmocom-generator ${D}${systemd_unitdir}/system-generators/

	install -d ${D}${sysconfdir}/sysmocom/configure.d
	install -m 0755 ${WORKDIR}/*.sh ${D}${sysconfdir}/sysmocom/configure.d/
}

FILES_${PN} += "${systemd_unitdir}/system-generators"
