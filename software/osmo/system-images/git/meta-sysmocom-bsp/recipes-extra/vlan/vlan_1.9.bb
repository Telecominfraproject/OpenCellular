DESCRIPTION = "802.1q vlan support program"
RRECOMMENDS_${PN} = "kernel-module-8021q"
LICENSE = "GPLv2+"
PR = "r4"

LIC_FILES_CHKSUM = "file://README;md5=ae5a24f54a98660ad86aad6c42052e48"

S = "${WORKDIR}/vlan/"

SRC_URI = "http://www.candelatech.com/~greear/vlan/vlan.${PV}.tar.gz"

inherit base

CCFLAGS = "-g -D_GNU_SOURCE -Wall -I${STAGING_INCDIR}"
LDLIBS = ""

do_compile() {
	${CC} ${CCFLAGS} -c vconfig.c
	${CC} ${CCFLAGS} ${LDFLAGS} -o vconfig vconfig.o ${LDLIBS}
}

do_install() {
	install -d "${D}/${sbindir}"
	install -m 755 "${S}/vconfig" "${D}/${sbindir}"
}


SRC_URI[md5sum] = "5f0c6060b33956fb16e11a15467dd394"
SRC_URI[sha256sum] = "3b8f0a1bf0d3642764e5f646e1f3bbc8b1eeec474a77392d9aeb4868842b4cca"
