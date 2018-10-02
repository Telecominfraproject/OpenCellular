HOMEPAGE = "http://www.sysmocom.de"
DESCRIPTION = "Set of tools to partition, flash and generate images to help development process with sysmobts2100"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

SRC_URI = "file://mkrootfsimg \
	  file://mksdcard \
	  file://uartboot"
PR = "r1"

do_deploy() {
        install -d ${DEPLOY_DIR_TOOLS}

        install -m 0755 ${WORKDIR}/mkrootfsimg ${DEPLOY_DIR_TOOLS}/mkrootfsimg-${PR}
        rm -f ${DEPLOY_DIR_TOOLS}/mkrootfsimg
        ln -sf ./mkrootfsimg-${PR} ${DEPLOY_DIR_TOOLS}/mkrootfsimg

	install -m 0755 ${WORKDIR}/mksdcard ${DEPLOY_DIR_TOOLS}/mksdcard-${PR}
        rm -f ${DEPLOY_DIR_TOOLS}/mksdcard
        ln -sf ./mksdcard-${PR} ${DEPLOY_DIR_TOOLS}/mksdcard

	install -m 0755 ${WORKDIR}/uartboot ${DEPLOY_DIR_TOOLS}/uartboot-${PR}
        rm -f ${DEPLOY_DIR_TOOLS}/uartboot
        ln -sf ./uartboot-${PR} ${DEPLOY_DIR_TOOLS}/uartboot
}

addtask deploy before do_package after do_install
