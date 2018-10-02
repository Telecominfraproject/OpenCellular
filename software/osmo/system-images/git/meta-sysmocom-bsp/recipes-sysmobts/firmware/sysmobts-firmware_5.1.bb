COMPATIBLE_MACHINE = "(sysmobts-v2)"
PACKAGE_ARCH = "sysmobts-v2"
FIRMWARE-VERSION = "superfemto_v${PV}"
PR = "r1.${INC_PR}"

SRC_URI[md5sum] = "21890090cbc5d5ed6661533835c38a8e"
SRC_URI[sha256sum] = "4eea3eb892103d2a73b944b4deb32a1d87859415c51b1fef776f15ff413d58b2"

require ${PN}.inc

do_install() {
	install -d ${D}/lib/firmware/

	install -m 0666 ${S}/Image/Dsp/superfemto.out ${D}/lib/firmware/sysmobts-v2.out
	install -m 0666 ${S}/Image/Fpga/superfemto.bit ${D}/lib/firmware/sysmobts-v2.bit
}

FILES_${PN} = "/lib/firmware/sysmobts-v2*"
