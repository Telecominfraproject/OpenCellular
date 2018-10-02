LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://Licenses/README;md5=0507cd7da8e7ad6d6701926ec9b84c95"

NRW_LC15_MIRROR ??= "gitlab.com/nrw_litecell15"

inherit gitver-pkg gitver-repo

# Should match the one in u-boot.inc
INC_PR  ??= "r1"

REPODIR   = "${THISDIR}"
REPOFILE  = "u-boot-litecell15-2015.07.inc"
INC_PR   := "${INC_PR}.${REPOGITFN}"

PV   = "2015.07+git${SRCPV}"
PKGV = "${PKGGITV}"

DEV_BRANCH  = "${@ 'nrw/litecell15-next' if d.getVar('NRW_BSP_DEVEL', False) == "next" else 'nrw/litecell15'}"
DEV_SRCREV  = "${AUTOREV}"
DEV_SRCURI := "git://${NRW_LC15_MIRROR}/u-boot.git;protocol=https;branch=${DEV_BRANCH}"

REL_BRANCH  = "nrw/litecell15"
REL_SRCREV  = "e2b1ddd84d72d8c57815265860ae58f6b170551c"
REL_SRCURI := "git://${NRW_LC15_MIRROR}/u-boot.git;protocol=https;branch=${REL_BRANCH}"

BRANCH  = "${@ '${DEV_BRANCH}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_BRANCH}'}"
SRCREV  = "${@ '${DEV_SRCREV}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_SRCREV}'}"
SRC_URI = "${@ '${DEV_SRCURI}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_SRCURI}'}"

SRC_URI += "file://0001-fw_env-missing-header.patch"
SRC_URI += "file://fix-build-error-under-gcc6.patch"
SRC_URI += "file://0001-Add-sysmocom-specific-tweaks.patch"

addtask showversion after do_compile before do_install
do_showversion() {
    bbplain "${PN}: ${PKGGITV} => ${BRANCH}:${PKGGITH}"
}

do_configure_prepend() {
    sed -i -e 's/SUBLEVEL =.*/SUBLEVEL = ${PKGGITN}/g' ${S}/Makefile
    sed -i -e 's/EXTRAVERSION =.*/EXTRAVERSION = -lc15/g' ${S}/Makefile
}
