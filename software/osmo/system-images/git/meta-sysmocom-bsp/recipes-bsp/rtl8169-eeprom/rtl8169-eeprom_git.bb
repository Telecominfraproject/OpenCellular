SUMMARY = "small utility to configure MAC-addresses on ALIX/APU boards"
HOMEPAGE = ""
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=b234ee4d69f5fce4486a80fdaf4a4263"
DEPENDS = "pciutils"

SRCREV = "2052514dc99575140af40b25e41c438c98eb9b48"
SRC_URI = "git://git.sysmocom.de/rtl8168-eeprom;protocol=git;branch=master"
PV = "v0.0.1+git${SRCPV}"
PR = "r0"

S = "${WORKDIR}/git"

inherit autotools pkgconfig
B = "${S}"

CFLAGS += ""
LDFLAGS += ""

CFLAGS += "`pkg-config --cflags libpci`"
LDFLAGS += "`pkg-config --libs libpci`"

do_configure() {
}

do_install() {
	install -d ${D}${sbindir}/
	install -m 0755 ${S}/rtl8168-eeprom ${D}${sbindir}/
}

