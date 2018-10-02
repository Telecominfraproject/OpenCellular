DESCRIPTION = "Package group for SOB"
LICENSE = "MIT"
LIC_FILES_CHKSUM = " \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
ALLOW_EMPTY_${PN} = "1"
PR = "r2"


RDEPENDS_${PN} = "\
	ifupdown vlan iproute2 iproute2-misc iputils \
"

