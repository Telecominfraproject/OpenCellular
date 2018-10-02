DESCRIPTION = "sysmocom sysmoBTS DSP and FPGA firmware"
SECTION = "kernel"
LICENSE = "CLOSED"

SRC_URI = "file://sysmobts-firmware-${FIRMWARE-VERSION}.tar.bz2"
S = "${WORKDIR}/sysmobts-firmware-${FIRMWARE-VERSION}"

INC_PR = "1"

do_compile() {
	:
}

PACKAGE_ARCH = "${MACHINE_ARCH}"
