DESCRIPTION = "UBL and second stage bootloader flasher for davinci"
HOMEPAGE = "http://www.hugovil.com/en/dvnixload/"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://src/main.c;beginline=1;endline=20;md5=f2b40a2eb5162af4c3cb20428e72f921"

SRCREV = "095c1cb55757c1f542370e5ba33de663c5d68ba4"
SRC_URI = "git://git.sysmocom.de/sysmo-bts/dvnixload;protocol=git;branch=master"
PV = "v0.2.6+git${SRCPV}"
PR = "r1"
S = "${WORKDIR}/git"

inherit autotools

BBCLASSEXTEND="native"


SRC_URI[md5sum] = "33308f47405c0e96a8248c7b1229dee5"
SRC_URI[sha256sum] = "5b76e9cb0ee843208c17053315926e0e168db8a89fe960655a0d0f4871e2b9da"

do_deploy() {
        install -d ${DEPLOY_DIR_TOOLS}
        install -m 0755 ${B}/src/dvnixload ${DEPLOY_DIR_TOOLS}/dvnixload-${PV}
        rm -f ${DEPLOY_DIR_TOOLS}/dvnixload
        ln -sf ./dvnixload-${PV} ${DEPLOY_DIR_TOOLS}/dvnixload

}

addtask deploy before do_package after do_install
