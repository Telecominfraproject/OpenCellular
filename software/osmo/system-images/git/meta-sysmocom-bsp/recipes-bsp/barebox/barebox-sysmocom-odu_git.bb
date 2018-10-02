require barebox.inc

RDEPENDS_${PN} += "${PN}-mlo"

SRCREV = "34a48171a699560d8a41d00d2c07ed37a79c00d8"
SRC_URI = " \
	git://git.sysmocom.de/barebox.git;branch=v2015.06 \
	file://defconfig \
	"

PV = "v2015.06+git${SRCPV}"
S = "${WORKDIR}/git"

BAREBOX_IMAGE ?= "barebox-${MACHINE}-${PKGV}-${PKGR}.img"
BAREBOX_SYMLINK ?= "barebox-${MACHINE}.img"

# generated using echo -n 'odu-stop' | sha1sum
BAREBOX_PASSWORD = "45cd62a2c4b834d6f8077db04f024aec070801db"

do_configure_append () {
	mkdir -p ${WORKDIR}/env/nv
	echo 5 > ${WORKDIR}/env/nv/login.timeout
}

do_deploy_append () {
	install -d ${DEPLOYDIR}
	install ${S}/images/barebox-am33xx-sysmocom-odu.img ${DEPLOYDIR}/${BAREBOX_IMAGE}
	cd ${DEPLOYDIR}
	rm -f ${BAREBOX_SYMLINK}
	ln -sf ${BAREBOX_IMAGE} ${BAREBOX_SYMLINK}
}
