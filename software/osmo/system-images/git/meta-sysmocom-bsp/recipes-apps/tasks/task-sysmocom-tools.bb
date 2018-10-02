DESCRIPTION = "Task for sysmocom external tools"
LICENSE = "MIT"
LIC_FILES_CHKSUM = " \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
ALLOW_EMPTY_${PN} = "1"
PR = "r12"

RDEPENDS_${PN} = "\
    lmsensors-scripts \
    dropbear \
    mtd-utils \
    screen \
    ethtool \
    ntpdate \
    wget \
    ca-cacert-rootcert \
    ipaccess-utils \
    abisip-find \
    sysmocom-backup \
    sysmocom-backup-default \
    sysmocom-systemd \
    sysmocom-configure \
    "
