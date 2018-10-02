DESCRIPTION = "Hardware health monitoring applications"
HOMEPAGE = "http://www.lm-sensors.org/"
DEPENDS = "sysfsutils virtual/libiconv"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=751419260aa954499f7abaabaa882bbe"
PR = "r5"
DEPENDS = "bison-native flex-native"
PACKAGE_ARCH = "${MACHINE_ARCH}"

SRC_URI = "http://pkgs.fedoraproject.org/repo/pkgs/lm_sensors/lm_sensors-${PV}.tar.bz2/829d88fb09d67723fbf42853eb84d1fd/lm_sensors-${PV}.tar.bz2"

SRC_URI[md5sum] = "829d88fb09d67723fbf42853eb84d1fd"
SRC_URI[sha256sum] = "bde7e1d8b473bca6528694b696668c4cd0a28515aef36b961e4f7d8a6b47e581"

S = "${WORKDIR}/lm_sensors-${PV}"

EXTRA_OEMAKE = 'LINUX=${STAGING_KERNEL_DIR} EXLDFLAGS="${LDFLAGS}" \
		MACHINE=${TARGET_ARCH} PREFIX=${prefix} CC="${CC}" AR="${AR}" \
		MANDIR="${mandir}"'

do_compile() {
	oe_runmake user PROG_EXTRA=sensors
}

do_install() {
	oe_runmake user_install DESTDIR=${D}
}

PACKAGES =+  "libsensors libsensors-dev libsensors-staticdev libsensors-dbg libsensors-doc"
PACKAGES =+ "lmsensors-sensors lmsensors-sensors-dbg lmsensors-sensors-doc"
PACKAGES =+ "lmsensors-scripts sensors-detect"

FILES_sensors-detect = "${sbindir}/sensors-detect"
RDEPENDS_sensors-detect += "perl"

FILES_lmsensors-scripts = "${bindir}/ddcmon ${sbindir}/fancontrol* ${sbindir}/pwmconfig"
RDEPENDS_lmsensors-scripts += "lmsensors-sensors bash"

FILES_lmsensors-sensors = "${bindir}/sensors ${sysconfdir}"
FILES_lmsensors-sensors-dbg += "${bindir}/.debug/sensors"
FILES_lmsensors-sensors-doc = "${mandir}/man1 ${mandir}/man5"
FILES_libsensors = "${libdir}/libsensors.so.*"
FILES_libsensors-dbg += "${libdir}/.debug"
FILES_libsensors-dev = "${libdir}/libsensors.so ${includedir}"
FILES_libsensors-staticdev = "${libdir}/libsensors.a"
FILES_libsensors-doc = "${mandir}/man3"
