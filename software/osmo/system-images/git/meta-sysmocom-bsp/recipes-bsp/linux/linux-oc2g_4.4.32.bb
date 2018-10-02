SECTION = "kernel"
DESCRIPTION = "Linux kernel for the OC-2G"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

require recipes-kernel/linux/linux-yocto.inc

KERNEL_IMAGETYPE = "zImage"

COMPATIBLE_MACHINE = "oc2g"

RDEPENDS_kernel-base += "kernel-devicetree"

KERNEL_DEVICETREE_oc2g = "oc2g.dtb oc2gplus.dtb"

RDEPENDS_kernel-devicetree += "update-alternatives-opkg"

LINUX_VERSION = "${PV}"
LINUX_VERSION_EXTENSION = "-oc2g"

RDEPENDS_kernel-image += "update-alternatives-opkg"

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}-${PV}:"

S = "${WORKDIR}/git"

NRW_OC2G_MIRROR ??= "git@gitlab.com/nrw_oc2g"

inherit gitver-pkg gitver-repo

REPODIR   = "${THISDIR}"
REPOFILE  = "linux-oc2g_4.4.32.bb"
PR       := "r${REPOGITFN}"

REPODIR   = "${THISDIR}/linux-oc2g-4.4.32"
REPOFILE  = ""
PR       := "${PR}.${REPOGITFN}"

PV   = "4.4.32+git${SRCPV}"
PKGV = "${PKGGITV}"

DEV_BRANCH  = "${@ 'nrw/oc2g-next' if d.getVar('NRW_BSP_DEVEL', False) == "next" else 'nrw/oc2g'}"
DEV_SRCREV  = "${AUTOREV}"
DEV_SRCURI := "git://${NRW_OC2G_MIRROR}/processor-sdk-linux.git;protocol=ssh;branch=${DEV_BRANCH}"

REL_BRANCH  = "nrw/oc2g"
REL_SRCREV  = "5a36597a52fe4fc24fc0d9f8a02e3c3ff30e6aff"
REL_SRCURI := "git://${NRW_OC2G_MIRROR}/processor-sdk-linux.git;protocol=ssh;branch=${REL_BRANCH}"

BRANCH  = "${@ '${DEV_BRANCH}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_BRANCH}'}"
SRCREV  = "${@ '${DEV_SRCREV}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_SRCREV}'}"
SRC_URI = "${@ '${DEV_SRCURI}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_SRCURI}'}"

addtask showversion after do_compile before do_install
do_showversion() {
    bbplain "${PN}: ${PKGGITV} => ${BRANCH}:${PKGGITH}"
}

do_configure_prepend() {
    sed -i -e 's/EXTRAVERSION =.*/EXTRAVERSION = .${PKGGITN}-oc2g/g' ${S}/Makefile
}

SRC_URI += "file://defconfig"
