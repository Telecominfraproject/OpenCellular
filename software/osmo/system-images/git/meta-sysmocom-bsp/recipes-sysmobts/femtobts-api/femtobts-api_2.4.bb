DESCRIPTION = "sysmoBTS Layer1 API header files"
SECTION = "kernel"
LICENSE = "CLOSED"

SRC_URI = "git://git.sysmocom.de/sysmo-bts/layer1-api.git;protocol=git;branch=master"
SRCREV = "superfemto_v2.4"
PV = "2.4"
PR = "r1"
S = "${WORKDIR}/git"


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
