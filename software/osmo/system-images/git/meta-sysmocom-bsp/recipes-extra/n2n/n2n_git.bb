SUMMARY = "n2n is a peer-to-peer L2-over-L3 VPN"
HOMEPAGE = "https://github.com/ntop/n2n"
LICENSE = "GPLv3+"
LIC_FILES_CHKSUM = "file://COPYING;md5=d2dd9497ff2aa79327dc88b6ce2b03cc"
DEPENDS = "openssl"
PV = "2.1.0+git${SRCPV}"
RRECOMMENDS_${PN} = "kernel-module-tun"

PR="r3"

SRC_URI = "git://github.com/ntop/n2n.git;branch=master \
	   file://edge.sh \
	   file://n2n-edge@.service \
	   file://0001-Makefile-Allow-cross-compilation-environment-to-spec.patch"
SRCREV= "${AUTOREV}"
S = "${WORKDIR}/git"

inherit systemd

do_compile() {
	CFLAGS="$CFLAGS $LDFLAGS"
	oe_runmake 
}

do_install() {
	oe_runmake install DESTDIR=${D}
	install -d ${D}/usr/share/n2n
	install -m 0755 ${WORKDIR}/edge.sh ${D}/usr/share/n2n
	install -d ${D}${systemd_system_unitdir}
	install -m 0644 ${WORKDIR}/n2n-edge@.service ${D}${systemd_system_unitdir}
}

FILES_${PN} += "${systemd_unitdir}"

#PACKAGES += "n2n-edge n2n-supernode"
#
#FILES_n2n_edge = "\
#	${sbindir}/edge \
#	${mandir}/man8/edge.8.gz \
#	"
#
#FILES_n2n_supernode = "\
#	${sbindir}/supernode \
#	${mandir}/man1/supernode.1.gz \
#	${mandir}/man7/n2n_v2.7.gz \
#	"
