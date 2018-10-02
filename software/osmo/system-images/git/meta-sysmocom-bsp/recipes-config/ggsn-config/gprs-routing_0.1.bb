DESCRIPTION = "OpenGGSN GPRS routing to the real world"
RDEPENDS_${PN} = "iptables kernel-module-ipt-masquerade"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "file://gprs_routing"

PR = "r1"

inherit update-rc.d

INITSCRIPT_PACKAGES = "${PN}"
INITSCRIPT_NAME_${PN} = "gprs_routing"
INITSCRIPT_PARAMS_${PN} = "defaults 28 28"


do_install() {
	install -d ${D}${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/gprs_routing ${D}${sysconfdir}/init.d/gprs_routing
}
