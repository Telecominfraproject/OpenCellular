SUMMARY = "nfacct is the command line tool to create/retrieve/delete accounting objects"
HOMEPAGE = "http://www.netfilter.org/projects/nfacct/downloads.html"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://COPYING;md5=8ca43cbc842c2336e835926c2166c28b"

DEPENDS = "libmnl libnetfilter-acct"

SRC_URI = "http://www.netfilter.org/projects/${PN}/files/${PN}-${PV}.tar.bz2;name=tar"
SRC_URI[tar.md5sum] = "992e863409d144350dbc8f0554a0f478"
SRC_URI[tar.sha256sum] = "81ef261616f313372a957431d17c5a0334984f06ceea190cf390479bf043e7c4"

RDEPENDS_${PN} += "kernel-module-xt-nfacct kernel-module-nfnetlink-acct"

inherit autotools pkgconfig
