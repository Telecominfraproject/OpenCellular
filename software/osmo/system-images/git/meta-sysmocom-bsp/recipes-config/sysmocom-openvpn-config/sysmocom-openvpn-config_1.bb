HOMEPAGE = "http://www.sysmocom.de"
RDEPENDS_${PN} = "openvpn"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "file://sysmocom-vpn.conf"
PR = "r4"

CONFFILES_${PN} = "${sysconfdir}/openvpn/sysmocom-vpn.conf.off"
PACKAGE_ARCH = "all"

do_install() {
	install -d ${D}${sysconfdir}/openvpn
	install -m 0644 ${WORKDIR}/sysmocom-vpn.conf ${D}${sysconfdir}/openvpn/sysmocom-vpn.conf.off
}

# Always enable the OpenVPN service. This assumes that
# this package will never be inside the nornal sysmocom
# feed.
FILES_${PN} += "${systemd_unitdir}"
do_install_append() {
	install -d ${D}${systemd_system_unitdir}/multi-user.target.wants/
	ln -sf ../openvpn.service ${D}${systemd_system_unitdir}/multi-user.target.wants/
}
