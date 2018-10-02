DESCRIPTION = "Udev helper script for static device names on the sysmocom odu"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "file://odu-persistens-serial.rules"

PACKAGES = "${PN}"
FILES_${PN} = "${BASELIB}/udev/rules.d/*"
RDPEPENDS_${PN} = "udev"

do_install() {
	install -d ${D}/${BASELIB}/udev/rules.d
	install -m 0644 ${WORKDIR}/odu-persistens-serial.rules ${D}/${BASELIB}/udev/rules.d/70-odu-persistens-serial.rules
}
