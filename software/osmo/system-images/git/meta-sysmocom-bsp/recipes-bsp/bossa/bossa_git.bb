SUMMARY = "flash programming utility for Atmel's SAM family of flash-based ARM microcontrollers"
HOMEPAGE = "http://sourceforge.net/projects/b-o-s-s-a/"
LICENSE = "GPLv3"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=d32239bcb673463ab874e80d47fae504"
SRCREV = "05bfcc39bc0453c3028b1161175b95a81af7a901"
SRC_URI = "git://git.code.sf.net/p/b-o-s-s-a/code"
DEPENDS = "readline"

PV = "v0.0+git${SRCPV}"
PR = "r2"

S = "${WORKDIR}/git"

do_compile() {
	mkdir -p obj/arm-dis
	oe_runmake -f Makefile bin/bossac bin/bossash
}

do_install() {
	install -d ${D}${bindir}/
	install -m 0755 ${S}/bin/bossac ${D}${bindir}/bossac
	install -m 0755 ${S}/bin/bossash ${D}${bindir}/bossash
}
