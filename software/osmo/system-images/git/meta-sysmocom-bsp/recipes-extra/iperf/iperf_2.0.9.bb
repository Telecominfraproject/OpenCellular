DESCRIPTION = "Iperf is a tool to measure maximum TCP bandwidth, allowing the tuning of various parameters and UDP characteristics"
HOMEPAGE = "http://dast.nlanr.net/Projects/Iperf/"
SECTION = "console/network"
LICENSE = "BSD-2-Clause"
LIC_FILES_CHKSUM = "file://COPYING;md5=e8478eae9f479e39bc34975193360298"

SRC_URI = "${SOURCEFORGE_MIRROR}/iperf2/${PN}-${PV}.tar.gz \
           file://stdbool-compilation.patch \
"

SRC_URI[md5sum] = "351b018b71176b8cb25f20eef6a9e37c"
SRC_URI[sha256sum] = "db02911f35686e808ed247160dfa766e08ae3f59d1e7dcedef0ffb2a6643f0bf"

S = "${WORKDIR}/${PN}-${PV}"

inherit autotools pkgconfig

EXTRA_OECONF = "--exec-prefix=${STAGING_DIR_HOST}${layout_exec_prefix}"
