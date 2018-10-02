COMPATIBLE_MACHINE = "(sysmobts-v2)"
PACKAGE_ARCH = "sysmobts-v2"
FIRMWARE-VERSION = "superfemto_v${PV}"
PR = "r1.${INC_PR}"

SRC_URI[md5sum] = "69993545decb8bdf39fa4dd9e5103ef9"
SRC_URI[sha256sum] = "8b2b19475a298299ef2de1061940ac690c447fa7b5d22b4fdb9dfd62bdc3c4a8"

require ${PN}.inc

do_install() {
	install -d ${D}/lib/firmware/

	install -m 0666 ${S}/Image/Dsp/superfemto.out ${D}/lib/firmware/sysmobts-v2.out
	install -m 0666 ${S}/Image/Fpga/superfemto.bit ${D}/lib/firmware/sysmobts-v2.bit
}

FILES_${PN} = "/lib/firmware/sysmobts-v2*"
