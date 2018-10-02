require barebox.inc

RDEPENDS_${PN} += "${PN}-mlo"

SRCREV = "ec82959f054af3e4a27267290905cfd895f75331"
SRC_URI = " \
	git://git.sysmocom.de/barebox.git;branch=v2015.06 \
        file://0001-OWHW-HACK-hard-code-the-bootstate-backend-node.patch \
	file://defconfig \
	"

PV = "v2015.06+git${SRCPV}"
S = "${WORKDIR}/git"

BAREBOX_IMAGE ?= "barebox-${MACHINE}-${PKGV}-${PKGR}.img"
BAREBOX_SYMLINK ?= "barebox-${MACHINE}.img"

# generated using echo -n 'owhw-stop' | sha1sum
BAREBOX_PASSWORD = "d797c986b04cdcb86206a990908e27f8c3ae96b4"

do_configure_append () {
	mkdir -p ${WORKDIR}/env/nv
	echo 5 > ${WORKDIR}/env/nv/login.timeout
}

do_deploy_append () {
	install -d ${DEPLOYDIR}
	install ${S}/images/barebox-am33xx-gsmk-owhw.img ${DEPLOYDIR}/${BAREBOX_IMAGE}
	cd ${DEPLOYDIR}
	rm -f ${BAREBOX_SYMLINK}
	ln -sf ${BAREBOX_IMAGE} ${BAREBOX_SYMLINK}
}
