SUMMARY = "small utility to set configuration of USB2514 hub chip on the sysmo-odu"
HOMEPAGE = ""
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://${WORKDIR}/usb2514.c;beginline=1;endline=18;md5=3b8421a1c05d21add65cc20fccfa29cd"
DEPENDS += "lmsensors-apps"

PR = "r4"

SRC_URI = "file://usb2514.c \
           file://gpio_usb2514 \
           file://odu-gpiotool \
           file://i2c-dev.h \
           file://usb2514.service \
          "

SRC_URI[md5sum] = ""
SRC_URI[sha256sum] = ""

do_compile() {
	${CC} -o ${WORKDIR}/usb2514 ${WORKDIR}/usb2514.c
}

do_install() {
	install -d ${D}${bindir}/
	install -m 0755 ${WORKDIR}/usb2514 ${D}${bindir}/
	install -m 0755 ${WORKDIR}/gpio_usb2514 ${D}${bindir}/
	install -m 0755 ${WORKDIR}/odu-gpiotool ${D}${bindir}/
	install -d ${D}${systemd_system_unitdir}/multi-user.target.wants/
	install -m 0644 /${WORKDIR}/usb2514.service ${D}${systemd_system_unitdir}/
	ln -sf ../usb2514.service ${D}${systemd_system_unitdir}/multi-user.target.wants/
}

FILES_${PN} += "${systemd_unitdir}"
