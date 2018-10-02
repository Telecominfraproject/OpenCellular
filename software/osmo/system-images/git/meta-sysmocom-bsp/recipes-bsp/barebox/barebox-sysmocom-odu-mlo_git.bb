require barebox.inc

SRCREV = "34a48171a699560d8a41d00d2c07ed37a79c00d8"
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
	install ${S}/images/barebox-am33xx-sysmocom-odu-mlo.img ${DEPLOYDIR}/${BAREBOX_IMAGE}
	cd ${DEPLOYDIR}
	rm -f ${BAREBOX_SYMLINK}
	ln -sf ${BAREBOX_IMAGE} ${BAREBOX_SYMLINK}
}
