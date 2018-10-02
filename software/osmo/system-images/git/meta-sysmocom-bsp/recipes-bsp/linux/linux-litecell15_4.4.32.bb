SECTION = "kernel"
DESCRIPTION = "Linux kernel for the LiteCell 1.5"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

require recipes-kernel/linux/linux-yocto.inc

KERNEL_IMAGETYPE = "zImage"

COMPATIBLE_MACHINE = "(litecell15|sysmobts2100)"

RDEPENDS_kernel-base += "kernel-devicetree"

KERNEL_DEVICETREE_litecell15 = "litecell15.dtb"
KERNEL_DEVICETREE_sysmobts2100 = "litecell15.dtb"

RDEPENDS_kernel-devicetree += "update-alternatives-opkg"

LINUX_VERSION = "${PV}"
LINUX_VERSION_EXTENSION = "-lc15"

RDEPENDS_kernel-image += "update-alternatives-opkg"

FILESEXTRAPATHS_prepend := "${THISDIR}/${PN}-${PV}:"

S = "${WORKDIR}/git"

NRW_LC15_MIRROR ??= "gitlab.com/nrw_litecell15"

inherit gitver-pkg gitver-repo

REPODIR   = "${THISDIR}"
REPOFILE  = "linux-litecell15_4.4.32.bb"
PR       := "r${REPOGITFN}"

REPODIR   = "${THISDIR}/files"
REPOFILE  = "."
PR       := "${PR}.${REPOGITFN}"

PV   = "4.4.32+git${SRCPV}"
PKGV = "${PKGGITV}"

DEV_BRANCH  = "${@ 'nrw/litecell15-next' if d.getVar('NRW_BSP_DEVEL', False) == "next" else 'nrw/litecell15'}"
DEV_SRCREV  = "${AUTOREV}"
DEV_SRCURI := "git://${NRW_LC15_MIRROR}/processor-sdk-linux.git;protocol=https;branch=${DEV_BRANCH}"

REL_BRANCH  = "nrw/litecell15"
REL_SRCREV  = "a54d64a4be25d87032a8600b97b271f255587844"
REL_SRCURI := "git://${NRW_LC15_MIRROR}/processor-sdk-linux.git;protocol=https;branch=${REL_BRANCH}"

BRANCH  = "${@ '${DEV_BRANCH}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_BRANCH}'}"
SRCREV  = "${@ '${DEV_SRCREV}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_SRCREV}'}"
SRC_URI = "${@ '${DEV_SRCURI}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_SRCURI}'}"

addtask showversion after do_compile before do_install
do_showversion() {
    bbplain "${PN}: ${PKGGITV} => ${BRANCH}:${PKGGITH}"
}

do_configure_prepend() {
    sed -i -e 's/EXTRAVERSION =.*/EXTRAVERSION = .${PKGGITN}-lc15/g' ${S}/Makefile
}

SRC_URI += "file://defconfig \
            file://0001-litecell15.dts-Set-default-let-trigger-to-none.patch"

# autoload defaults
module_autoload_nrw_clkerr = "nrw-clkerr"
module_autoload_nrw_vswr = "nrw-vswr"
module_autoload_rpmsg_proto = "rpmsg-proto"
module_autoload_rpmsg_rpc = "rpmsg-rpc"
module_autoload_iio_hwmon = "iio-hwmon"
module_autoload_ntc_thermistor = "ntc-thermistor"

KERNEL_MODULE_PROBECONF_append = "adl5501 configfs fpgadl iio_hwmon industrialio industrialio-buffer-cb input-polldev mcp47x6 nrw_clkerr nrw_vswr ntc_thermistor omap_remoteproc rpmsg_proto rpmsg_rpc xilinx-xadc"
KERNEL_MODULE_AUTOLOAD_append = "adl5501 configfs fpgadl iio_hwmon industrialio industrialio-buffer-cb input-polldev mcp47x6 nrw_clkerr nrw_vswr ntc_thermistor omap_remoteproc rpmsg_proto rpmsg_rpc xilinx-xadc"

RDEPENDS_kernel-module-omap-remoteproc += "lc15-firmware"
RDEPENDS_kernel-module-fpgadl += "lc15-firmware"
