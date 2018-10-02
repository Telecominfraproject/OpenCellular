DESCRIPTION = "Task for sysmocom"
LICENSE = "MIT"
LIC_FILES_CHKSUM = " \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
DEPENDS = "virtual/kernel"
ALLOW_EMPTY_${PN} = "1"
PR = "r1"

RDEPENDS_${PN} = "\
    task-sysmocom-tools \
    osmo-bsc-sccplite \
    osmo-bsc-nat \
    osmo-gbproxy \
    osmo-nitb \
    "
