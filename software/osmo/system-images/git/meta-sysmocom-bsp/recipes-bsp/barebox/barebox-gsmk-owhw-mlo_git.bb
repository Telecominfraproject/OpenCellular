require barebox.inc

SRCREV = "ce8849b03a40718fdaa9d7fc30312eeeb0fafcac"
SRC_URI = " \
	git://git.sysmocom.de/barebox.git;branch=v2015.06 \
	file://defconfig \
	"

PV = "v2015.06+git${SRCPV}"
S = "${WORKDIR}/git"

BAREBOX_IMAGE ?= "barebox-${MACHINE}-mlo-${PKGV}-${PKGR}.img"
BAREBOX_SYMLINK ?= "barebox-${MACHINE}-mlo.img"

do_deploy () {
	install -d ${DEPLOYDIR}
	install ${S}/images/barebox-am33xx-gsmk-owhw-mlo.img ${DEPLOYDIR}/${BAREBOX_IMAGE}
	cd ${DEPLOYDIR}
	rm -f ${BAREBOX_SYMLINK}
	ln -sf ${BAREBOX_IMAGE} ${BAREBOX_SYMLINK}
}
