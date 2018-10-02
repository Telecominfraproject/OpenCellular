LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=0507cd7da8e7ad6d6701926ec9b84c95"

NRW_OC2G_MIRROR ??= "git@gitlab.com/nrw_oc2g"

inherit gitver-pkg gitver-repo

# Should match the one in u-boot.inc
INC_PR  ??= "r1"

REPODIR   = "${THISDIR}"
REPOFILE  = "u-boot-oc2g-2015.07.inc"
INC_PR   := "${INC_PR}.${REPOGITFN}"

PV   = "2015.07+git${SRCPV}"
PKGV = "${PKGGITV}"

DEV_BRANCH  = "${@ 'nrw/oc2g-next' if d.getVar('NRW_BSP_DEVEL', False) == "next" else 'nrw/oc2g'}"
DEV_SRCREV  = "${AUTOREV}"
DEV_SRCURI := "git://${NRW_OC2G_MIRROR}/u-boot.git;protocol=ssh;branch=${DEV_BRANCH}"

REL_BRANCH  = "nrw/oc2g"
REL_SRCREV  = "f8da7e9b975e1806b6cb896600b6a67c5fcf4afd"
REL_SRCURI := "git://${NRW_OC2G_MIRROR}/u-boot.git;protocol=ssh;branch=${REL_BRANCH}"

BRANCH  = "${@ '${DEV_BRANCH}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_BRANCH}'}"
SRCREV  = "${@ '${DEV_SRCREV}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_SRCREV}'}"
SRC_URI = "${@ '${DEV_SRCURI}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_SRCURI}'}"

SRC_URI += "file://0001-fw_env-missing-header.patch"
SRC_URI += "file://fix-build-error-under-gcc6.patch"

addtask showversion after do_compile before do_install
do_showversion() {
    bbplain "${PN}: ${PKGGITV} => ${BRANCH}:${PKGGITH}"
}

do_configure_prepend() {
    sed -i -e 's/SUBLEVEL =.*/SUBLEVEL = ${PKGGITN}/g' ${S}/Makefile
    sed -i -e 's/EXTRAVERSION =.*/EXTRAVERSION = -oc2g/g' ${S}/Makefile
}
