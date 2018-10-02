SUMMARY = "Debian ifup/ifdown"
DESCRIPTION = "The core network utilities for debian. In contrast to busybox \
they can manage bridges and vlans."
HOMEPAGE = "http://anonscm.debian.org/hg/collab-maint/ifupdown/"
LICENSE = "GPLv2"

LIC_FILES_CHKSUM = "file://debian/copyright;md5=7adfbe801102d1e7e6bfdd3f03754efa"

SRC_URI = "https://launchpadlibrarian.net/194033720/ifupdown_${PV}.tar.xz \
	file://busybox-yocto-compat.patch \
	file://defn2-c-man-don-t-rely-on-dpkg-architecture-to-set-a.patch "

SRC_URI[md5sum] = "bb204ae2fa4171d6f1de4097f4570a7d"
SRC_URI[sha256sum] = "8a0647c59ee0606f5da9205c5b3c5b000fea98fe39348f6bb2cba5fecfc51090"

CFLAGS += "-D'IFUPDOWN_VERSION="0.7"'"

PR = "r2"

do_configure() {
	chmod a+rx makecdep.sh makenwdep.sh
}

do_install() {
        install -m 0755 -d     ${D}${base_sbindir}
        install -m 0755 ifup   ${D}${base_sbindir}/ifup.${BPN}
        ln -s ${base_sbindir}/ifup.${BPN} ${D}${base_sbindir}/ifdown.${BPN}
        ln -s ${base_sbindir}/ifup.${BPN} ${D}${base_sbindir}/ifquery
        install -D -m 0755 settle-dad.sh ${D}/lib/ifupdown/settle-dad.sh
}

inherit update-alternatives

ALTERNATIVE_PRIORITY = "100"
ALTERNATIVE_${PN} = "ifup ifdown"

ALTERNATIVE_LINK_NAME[ifup] = "${base_sbindir}/ifup"
ALTERNATIVE_TARGET[ifup] = "${base_sbindir}/ifup.${BPN}"


ALTERNATIVE_LINK_NAME[ifdown] = "${base_sbindir}/ifdown"
ALTERNATIVE_TARGET[ifdown] = "${base_sbindir}/ifdown.${BPN}"

FILES_${PN} += "/lib/ifupdown/settle-dad.sh"
