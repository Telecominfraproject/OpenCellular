DESCRIPTION = "sysmocom OsmoBTS"
LICENSE = "AGPLv3"
LIC_FILES_CHKSUM = "file://COPYING;md5=73f1eb20517c55bf9493b7dd6e480788"

SRC_URI = "git://git.osmocom.org/osmo-bts.git;protocol=git;branch=master;destsuffix=git"
SRCREV = "33da462a2bf37f2688d79530b11f9e65b5c93502"
PV = "0.8.1+git${SRCPV}"
PR = "r0.${META_TELEPHONY_OSMO_INC}"
S = "${WORKDIR}/git"

DEPENDS = "libosmocore libosmo-abis femtobts-api gpsd"
DEPENDS_append_sysmobts-v2 = " femtobts-api"
DEPENDS_append_sysmobts2100 = " lc15-firmware"

RDEPENDS_${PN} += "coreutils"

RDEPENDS_${PN}_append_sysmobts-v2 = " sysmobts-firmware (>= 5.1)"
RCONFLICTS_${PN}_append_sysmobts-v2 = " sysmobts-firmware (< 5.1)"

RDEPENDS_${PN}_append_sysmobts2100 = " lc15-firmware"

EXTRA_OECONF_sysmobts-v2 += "--enable-sysmocom-bts --enable-sysmobts-calib"
EXTRA_OECONF_sysmobts2100 += "--enable-litecell15"

inherit autotools pkgconfig systemd

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_AUTO_ENABLE_${PN}="enable"

# Select the API version
inherit femtobts_api
CPPFLAGS_append_sysmobts-v2 = " ${BTS_HW_VERSION} "

do_install_append() {
	install -d ${D}${sysconfdir}/osmocom
	install -d ${D}/${systemd_system_unitdir}
}

do_install_append_sysmobts-v2() {
	install -m 0660 ${S}/doc/examples/sysmo/osmo-bts.cfg ${D}${sysconfdir}/osmocom

	# Install systemd and enable on sysinit

	install -m 0660 ${S}/doc/examples/sysmo/sysmobts-mgr.cfg ${D}${sysconfdir}/osmocom
	install -m 0644 ${S}/contrib/sysmobts-mgr.service ${D}${systemd_system_unitdir}/
	install -m 0644 ${S}/contrib/osmo-bts-sysmo.service ${D}${systemd_system_unitdir}/
}

do_install_append_sysmobts2100() {
	install -m 0660 ${S}/doc/examples/litecell15/osmo-bts.cfg ${D}${sysconfdir}/osmocom

	# ensure consistent naming
	cp ${D}/${bindir}/lc15bts-util ${D}/${bindir}/sysmobts-util
	cp ${D}/${bindir}/lc15bts-mgr ${D}/${bindir}/sysmobts-mgr

	# Install systemd and enable on sysinit
	install -m 0644 ${S}/contrib/lc15bts-mgr.service ${D}${systemd_system_unitdir}/lc15bts-mgr.service
	install -m 0660 ${S}/doc/examples/litecell15/lc15bts-mgr.cfg ${D}${sysconfdir}/osmocom/
	install -m 0644 ${S}/contrib/osmo-bts-lc15.service ${D}${systemd_system_unitdir}/
}

SYSTEMD_SERVICE_${PN}_append_sysmobts-v2 = "sysmobts-mgr.service osmo-bts-sysmo.service"
SYSTEMD_SERVICE_${PN}_append_sysmobts2100 = "lc15bts-mgr.service osmo-bts-lc15.service"

CONFFILES_${PN} = "${sysconfdir}/osmocom/osmo-bts.cfg"
CONFFILES_${PN}_append_sysmobts-v2 = " ${sysconfdir}/osmocom/sysmobts-mgr.cfg"

# somehow it seems not posible to use _append constructs on PACKAGES
#PACKAGES_append_sysmobts-v2 = " osmo-bts-remote sysmobts-calib sysmobts-util"
PACKAGES =+ "osmo-bts-remote sysmobts-calib sysmobts-util"

FILES_osmo-bts-remote_sysmobts-v2 = " ${bindir}/osmo-bts-sysmo-remote "
FILES_sysmobts-calib_sysmobts-v2 = " ${bindir}/sysmobts-calib "
FILES_sysmobts-util = " ${bindir}/sysmobts-util "
