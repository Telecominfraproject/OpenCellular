DESCRIPTION = "sysmocom sysmoBTS 2050 board management utility"
SECTION = "kernel"
LICENSE = "CLOSED"

SRC_URI = "file://sbts2050-util-${FIRMWARE-VERSION}.tar.bz2"
S = "${WORKDIR}/sbts2050-util-${FIRMWARE-VERSION}"

INC_PR = "1"

do_compile() {
	:
}

COMPATIBLE_MACHINE = "sysmobts-v2"
PACKAGE_ARCH = "${MACHINE_ARCH}"

INSANE_SKIP_${PN} += "already-stripped ldflags"
