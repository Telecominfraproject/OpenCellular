SUMMARY = "small utility to set configuration of USB2514 hub chip on the sysmo-odu"
HOMEPAGE = ""
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://LICENSE;md5=e6868d1ce8f058ecc5658ecebb086636"

SRCREV = "1103417819fe855d8d0d3e6a277870679ad01bd2"
SRC_URI = "git://github.com/mlaurijsse/linux-mpu9150.git"

PV = "v0.0+git${SRCPV}"
PR = "r1a"

S = "${WORKDIR}/git"

do_compile() {
	oe_runmake -f Makefile-native
}

do_install() {
	install -d ${D}${bindir}/
	install -m 0755 ${S}/imu ${D}${bindir}/mpu9150-imu
	install -m 0755 ${S}/imucal ${D}${bindir}/mpu9150-imu-cal
}
