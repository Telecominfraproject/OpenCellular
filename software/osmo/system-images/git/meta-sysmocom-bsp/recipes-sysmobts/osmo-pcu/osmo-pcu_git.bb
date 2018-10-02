DESCRIPTION = "Osmocom PCU for sysmoBTS"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"

SRC_URI = "git://git.osmocom.org/osmo-pcu.git;protocol=git;branch=master"
SRCREV = "f1a334be63ef58a848fd141a88b988c843ba294e"
PV = "0.5.0+git${SRCPV}"
PR = "r0.${META_TELEPHONY_OSMO_INC}"
S = "${WORKDIR}/git"

DEPENDS = "libosmocore osmo-bts"
DEPENDS_append_sysmobts-v2 = " femtobts-api"
DEPENDS_append_litecell15   = " lc15-firmware"
DEPENDS_append_sysmobts2100 = " lc15-firmware"

# This implements PCU Interface v8 (GPRS RSSI)
RDEPENDS_${PN} = "osmo-bts (>= 0.8.0)"

EXTRA_OECONF_sysmobts-v2 += "--enable-sysmocom-dsp"
EXTRA_OECONF_litecell15   += "--enable-lc15bts-phy"
EXTRA_OECONF_sysmobts2100 += "--enable-lc15bts-phy"

inherit autotools pkgconfig systemd

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_AUTO_ENABLE_${PN}="enable"
SYSTEMD_SERVICE_${PN} = "osmo-pcu.service"

# Select the API version
inherit femtobts_api
CPPFLAGS_append_sysmobts-v2 = " ${BTS_HW_VERSION} "

do_install_append() {
        # Install systemd and enable on sysinit
	install -d ${D}/${systemd_system_unitdir}
        install -m 0644 ${S}/contrib/osmo-pcu.service ${D}${systemd_system_unitdir}/
}

CONFFILES_${PN} = "${sysconfdir}/osmocom/osmo-pcu.cfg"
