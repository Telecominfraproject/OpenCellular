DESCRIPTION = "Task for E1 based sysmocom"
LICENSE = "MIT"
LIC_FILES_CHKSUM = " \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
ALLOW_EMPTY_${PN} = "1"
PR = "r2"

RDEPENDS_${PN} = "\
    dahdi-linux \
    dahdi-firmware \
    dahdi-tools \
    "
