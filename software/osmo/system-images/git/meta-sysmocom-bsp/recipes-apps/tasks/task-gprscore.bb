DESCRIPTION = "Task for GPRS core network"
LICENSE = "MIT"
LIC_FILES_CHKSUM = " \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
DEPENDS = "virtual/kernel"
ALLOW_EMPTY_${PN} = "1"
PR = "r1"

RDEPENDS_${PN} = "\
    osmo-ggsn \
    gprs-routing \
    osmo-sgsn \
    "
