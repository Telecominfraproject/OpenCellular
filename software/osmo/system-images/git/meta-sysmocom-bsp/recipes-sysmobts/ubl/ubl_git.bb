DESCRIPTION = "Davinci UBL for the sysmobts v1 and v2"
HOMEPAGE = "http://www.sysmocom.de"
SECTION = "bootloaders"
PRIORITY = "optional"

LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://ubl.c;beginline=1;endline=22;md5=806283bb3c475d9082e41f09020373ff"

SRC_URI = "git://git.sysmocom.de/sysmo-bts/ubl;protocol=git;branch=master"
SRCREV = "77aac3693b45df7154ada64341fd67e41f990f22"
PV = "v0.2.11+git${SRCPV}"
PR = "r3"
S = "${WORKDIR}/git"

inherit deploy

EXTRA_OEMAKE = "CROSS_COMPILE=${TARGET_PREFIX}"

UBL_IMAGE ?= "ubl-${MACHINE}-${PKGV}-${PKGR}.elf"
UBL_SYMLINK ?= "ubl-${MACHINE}.elf"

BOARD_NAME_sysmobts-v2 = "sysmobts_v2"

do_compile() {
	unset LDFLAGS
	unset CFLAGS
	unset CPPFLAGS
	oe_runmake BOARD=${BOARD_NAME} CC="$CC"
}

do_deploy() {
	install ${S}/ubl_${BOARD_NAME}.elf ${DEPLOYDIR}/${UBL_IMAGE}
	cd ${DEPLOYDIR}
	rm -f ${UBL_SYMLINK}
	ln -sf ${UBL_IMAGE} ${UBL_SYMLINK}
}

addtask deploy before do_build after do_compile
