SUMMARY = "Firmware files for Nuran Wireless Litecell 1.5 Mainboard"
LICENSE = "CLOSED"

NRW_OC2G_MIRROR ??= "gitlab.com/nrw_oc2g"

S = "${WORKDIR}/git"

inherit gitver-pkg gitver-repo

REPODIR   = "${THISDIR}"
REPOFILE  = "oc2g-firmware_git.bb"
PR       := "r${REPOGITFN}"

PV   = "git${SRCPV}"
PKGV = "${PKGGITV}"

DEV_BRANCH  = "${@ 'nrw/oc2g-next' if d.getVar('NRW_BSP_DEVEL', False) == "next" else 'nrw/oc2g'}"
DEV_SRCREV  = "${AUTOREV}"
DEV_SRCURI := "git://${NRW_OC2G_MIRROR}/oc2g-fw.git;protocol=https;branch=${DEV_BRANCH}"

REL_BRANCH  = "nrw/oc2g"
REL_SRCREV  = "326cd66adfba7b507161a4f197d3f9b59e001dad"
REL_SRCURI := "git://${NRW_OC2G_MIRROR}/oc2g-fw.git;protocol=https;branch=${REL_BRANCH}"

BRANCH  = "${@ '${DEV_BRANCH}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_BRANCH}'}"
SRCREV  = "${@ '${DEV_SRCREV}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_SRCREV}'}"
SRC_URI = "${@ '${DEV_SRCURI}' if d.getVar('NRW_BSP_DEVEL', False) else '${REL_SRCURI}'}"

addtask showversion after do_compile before do_install
do_showversion() {
    bbplain "${PN}: ${PKGGITV} => ${BRANCH}:${PKGGITH}"
}

inherit allarch

do_install() {
    install -d ${D}${includedir}/nrw
    install -d ${D}${includedir}/nrw/oc2g
    install -m 0644 ${S}/inc/nrw/oc2g/* ${D}${includedir}/nrw/oc2g

    install -d ${D}${base_libdir}/firmware
    install -m 0644 ${S}/bin/* ${D}${base_libdir}/firmware
}

INSANE_SKIP_${PN} = "arch"

FILES_${PN} = "${base_libdir}/firmware/*"
FILES_${PN}-dev = "${includedir}/nrw/oc2g/*"
