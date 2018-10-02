DESCRIPTION = "barebox state tool (dt)"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=9ac2e7cff1ddaf48b6eab6028f23ef88"
PR = "r4"

SRC_URI = "\
	git://git.pengutronix.de/git/tools/dt-utils.git \
	file://0001-barebox-state-fix-typo.patch \
"

SRC_URI_append_sysmocom-odu = "file://hardcode-layout-values.patch"
SRC_URI_append_sysmobts-v2 = "file://hardcode-layout-values.patch"

PACKAGES =+ "libdt-utils barebox-fdtdump"

FILES_libdt-utils = "${libdir}/libdt-utils.so.*"
FILES_barebox-fdtdump = "${bindir}/fdtdump"

S = "${WORKDIR}/git"

SRCREV = "f0bddb4f82deaf73cf20aeda5bbf64c50a59dd60"

DEPENDS = "udev"

inherit autotools pkgconfig gettext
