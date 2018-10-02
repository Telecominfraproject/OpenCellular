SUMMARY = "libnetfilter_acct is the userspace library providing interface to extended accounting infrastructure."
HOMEPAGE = "http://www.netfilter.org/projects/libnetfilter_acct/index.html"
LICENSE = "LGPLv2.1+"
LIC_FILES_CHKSUM = "file://COPYING;md5=4fbd65380cdd255951079008b364516c"
PV="1.0.2+git${SRCPV}"
PR = "r0"

DEPENDS = "libmnl"

SRC_URI = "git://git.netfilter.org/libnetfilter_acct;branch=master \
	   file://0001-add-JSON-output-format.patch \
	"
SRCREV = "a9fea38024e6bde9118cc12bc8417b207ffc4da9"
S = "${WORKDIR}/git"

inherit autotools pkgconfig
