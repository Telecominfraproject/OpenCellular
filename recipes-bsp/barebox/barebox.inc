SUMMARY = "Barebox - a bootloader that inherits the best of U-Boot and the Linux kernel"
HOMEPAGE = "http://www.barebox.org/"
SECTION = "bootloaders"
PROVIDES ?= "virtual/bootloader"
DEPENDS += "lzop-native"

LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=057bf9e50e1ca857d0eb97bfe4ba8e5d"

PACKAGE_ARCH = "${MACHINE_ARCH}"

inherit kernel-arch deploy

EXTRA_OEMAKE = "ARCH=arm CROSS_COMPILE=${STAGING_BINDIR_TOOLCHAIN}/${TARGET_PREFIX}"

do_configure_prepend() {
	cp ${WORKDIR}/defconfig ${S}/.config
	# do not compile tools when using KERNEL_CC/KERNEL_LD
	echo CONFIG_BAREBOXENV_TARGET=n >> ${S}/.config
	echo CONFIG_BAREBOXCRC32_TARGET=n >> ${S}/.config
	echo CONFIG_KERNEL_INSTALL_TARGET=n >> ${S}/.config
	echo CONFIG_IMD_TARGET=n >> ${S}/.config
	# password
	if [ -n "${BAREBOX_PASSWORD}" ]; then
		echo -n "${BAREBOX_PASSWORD}" > ${S}/.password
		echo CONFIG_PASSWORD_DEFAULT=\".password\" >> ${S}/.config
	fi
	# extra environment files
	sed -i -e 's,^\(CONFIG_DEFAULT_ENVIRONMENT_PATH=.*\)\"$,\1 ../env\",' ${S}/.config
	oe_runmake oldconfig
}

do_compile () {
	unset CFLAGS CPPFLAGS CXXFLAGS LDFLAGS MACHINE
	oe_runmake CC="${KERNEL_CC}" LD="${KERNEL_LD}"
}

do_deploy () {
	install -d ${DEPLOY_DIR_TOOLS}
	if [ -f ${S}/scripts/mkimage ]; then
		install -m 755 ${S}/scripts/mkimage ${DEPLOY_DIR_TOOLS}/mkimage-${PV}
		ln -sf mkimage-${PV} ${DEPLOY_DIR_TOOLS}/mkimage
	fi
	if [ -f ${S}/scripts/bareboxenv ]; then
		install -m 755 ${S}/scripts/bareboxenv ${DEPLOY_DIR_TOOLS}/bareboxenv-${PV}
		ln -sf bareboxenv-${PV} ${DEPLOY_DIR_TOOLS}/bareboxenv
	fi
}

addtask deploy before do_build after do_compile
