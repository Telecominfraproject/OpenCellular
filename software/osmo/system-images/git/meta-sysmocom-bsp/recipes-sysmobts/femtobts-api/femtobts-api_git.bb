DESCRIPTION = "sysmoBTS Layer1 API header files"
SECTION = "kernel"
LICENSE = "CLOSED"

SRC_URI = "git://git.sysmocom.de/sysmo-bts/layer1-api.git;protocol=git;branch=master"
SRCREV = "84e0cf46511f256ef84c0211f3f39a642acceb33"
SRCREV_sysmobts-v2 = "7f0d5697b85340877b127a25e0c8f2a5f5fe66d7"
PV = "${SRCREV}+git${SRCPV}"
PR = "r5"
S = "${WORKDIR}/git"

# The API is only a header, nothing to compile
RDEPENDS_${PN}-dev = ""


do_compile() {
	:
}

do_install() {
	install -d ${D}${includedir}/sysmocom/femtobts
	install -d ${D}${libdir}/pkgconfig

	install -m 0755 ${S}/include/* ${D}${includedir}/sysmocom/femtobts/
	echo "Name: Sysmocom sysmoBTS Layer1 API
Description: Sysmocom sysmoBTS Layer1 API
Versions: 2.0
Libs:
Cflags: -I${includedir}/sysmocom/femtobts" > ${D}${libdir}/pkgconfig/sysmocom-btsapi.pc
}
