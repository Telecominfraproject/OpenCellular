PRINC="3"

# We need networking to run as we have no native services yet
SYSTEMD_DISABLED_SYSV_SERVICES_remove = "networking dnsmasq"

S_THISDIR := "${@os.path.dirname(d.getVar('FILE', True))}"
FILESPATH =. "${@base_set_filespath(["${S_THISDIR}/${PN}"], d)}:"

SRC_URI += "file://alignment.service"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

do_install_append() {
	install -d ${D}${systemd_system_unitdir}/basic.target.wants
	install -d ${D}${systemd_system_unitdir}/sysinit.target.wants/

	install -m 0644 ${WORKDIR}/alignment.service ${D}${systemd_system_unitdir}
	ln -sf ../alignment.service ${D}${systemd_system_unitdir}/basic.target.wants/
	ln -sf ../alignment.service ${D}${systemd_system_unitdir}/sysinit.target.wants/
}

FILES_${PN} = "${systemd_system_unitdir} ${bindir}"
