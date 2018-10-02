DESCRIPTION = "Task for sysmocom"
LICENSE = "MIT"
LIC_FILES_CHKSUM = " \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
DEPENDS = "virtual/kernel"
ALLOW_EMPTY_${PN} = "1"
PR = "r16"

RDEPENDS_${PN} = "\
    task-sysmocom-tools \
    task-gprscore \
    osmo-bsc \
    osmo-mgw \
    osmo-msc \
    osmo-hlr \
    osmo-stp \
    ${@bb.utils.contains('DISTRO_FEATURES', 'iu', 'osmo-hnbgw', '', d)} \
    "
