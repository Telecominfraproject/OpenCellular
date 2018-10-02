require barebox.inc

SRCREV = "d87c27d7ab143d58f358df5722be2b9675103d2e"
SRC_URI = " \
	git://git.sysmocom.de/barebox.git;branch=v2015.06 \
	file://defconfig \
	"

PV = "v2015.06+git${SRCPV}"
S = "${WORKDIR}/git"

BAREBOX_ELF_IMAGE ?= "barebox-${MACHINE}-${PKGV}-${PKGR}.elf"
BAREBOX_ELF_SYMLINK ?= "barebox-${MACHINE}.elf"
BAREBOX_BIN_IMAGE ?= "barebox-${MACHINE}-${PKGV}-${PKGR}.bin"
BAREBOX_BIN_SYMLINK ?= "barebox-${MACHINE}.bin"

# generated using echo -n 'bts-stop' | sha1sum
BAREBOX_PASSWORD = "5a7ef8875df28cb95a0f833906f94df8573bcc5d"

# Provide a replacement for calling whoami
export KBUILD_BUILD_USER="poky"

do_configure_append () {
	mkdir -p ${WORKDIR}/env/nv
	echo 5 > ${WORKDIR}/env/nv/login.timeout
}

do_deploy_append () {
	install -d ${DEPLOYDIR}
	install ${S}/arch/arm/pbl/zbarebox ${DEPLOYDIR}/${BAREBOX_ELF_IMAGE}
	install ${S}/arch/arm/pbl/zbarebox.bin ${DEPLOYDIR}/${BAREBOX_BIN_IMAGE}
	cd ${DEPLOYDIR}
	rm -f ${BAREBOX_ELF_SYMLINK}
	rm -f ${BAREBOX_BIN_SYMLINK}
	ln -sf ${BAREBOX_ELF_IMAGE} ${BAREBOX_ELF_SYMLINK}
	ln -sf ${BAREBOX_BIN_IMAGE} ${BAREBOX_BIN_SYMLINK}
}
