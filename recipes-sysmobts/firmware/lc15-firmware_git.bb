SUMMARY = "Firmware files for Nuran Wireless Litecell 1.5 Mainboard"
LICENSE = "CLOSED"

NRW_LC15_MIRROR ??= "gitlab.com/nrw_litecell15"

S = "${WORKDIR}/git"

inherit gitver-pkg gitver-repo

REPODIR   = "${THISDIR}"
REPOFILE  = "lc15-firmware_git.bb"
PR       := "r${REPOGITFN}"

PV   = "git${SRCPV}"
PKGV = "${PKGGITV}"

DEV_BRANCH  = "${@ 'nrw/litecell15-next' if d.getVar('NRW_BSP_DEVEL', False) == "next" else 'nrw/litecell15'}"
DEV_SRCREV  = "${AUTOREV}"
DEV_SRCURI := "git://${NRW_LC15_MIRROR}/litecell15-fw.git;protocol=https;branch=${DEV_BRANCH}"

REL_BRANCH  = "nrw/litecell15"
REL_SRCREV  = "a989c45337e3645a235764165514963e55559fd6"
REL_SRCURI := "git://${NRW_LC15_MIRROR}/litecell15-fw.git;protocol=https;branch=${REL_BRANCH}"

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
    install -d ${D}${includedir}/nrw/litecell15
    install -m 0644 ${S}/inc/nrw/litecell15/* ${D}${includedir}/nrw/litecell15

    install -d ${D}${base_libdir}/firmware
    install -m 0644 ${S}/bin/* ${D}${base_libdir}/firmware
}

INSANE_SKIP_${PN} = "arch"

FILES_${PN} = "${base_libdir}/firmware/*"
FILES_${PN}-dev = "${includedir}/nrw/litecell15/*"
