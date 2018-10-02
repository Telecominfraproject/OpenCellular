SUMMARY = "nfacct is the command line tool to create/retrieve/delete accounting objects"
HOMEPAGE = "http://www.netfilter.org/projects/nfacct/downloads.html"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=8ca43cbc842c2336e835926c2166c28b"
PV="1.0.1+git${SRCPV}"

PR="r1"

DEPENDS = "libmnl libnetfilter-acct"

SRC_URI = "git://git.netfilter.org/nfacct;branch=master \
	   file://0001-Add-JSON-output-formatting-to-nfacct-utility.patch \
	"
SRCREV = "4437682babe86de7435d4fc839437f99e998b79c"
S = "${WORKDIR}/git"

RDEPENDS_${PN} += "kernel-module-xt-nfacct kernel-module-nfnetlink-acct"

inherit autotools pkgconfig
