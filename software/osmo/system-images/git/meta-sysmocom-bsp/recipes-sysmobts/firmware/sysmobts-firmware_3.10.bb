COMPATIBLE_MACHINE = "(sysmobts-v2)"
PACKAGE_ARCH = "sysmobts-v2"
FIRMWARE-VERSION = "superfemto_v${PV}"
PR = "r9.${INC_PR}"
RRCONFLICTS_${PN} = "osmo-bts (< 0.4.2)"

require ${PN}.inc

do_install() {
	install -d ${D}/lib/firmware/

	install -m 0666 ${S}/Image/Dsp/superfemto.out ${D}/lib/firmware/sysmobts-v2.out
	install -m 0666 ${S}/Image/Fpga/superfemto.bit ${D}/lib/firmware/sysmobts-v2.bit
}

FILES_${PN} = "/lib/firmware/sysmobts-v2*"
