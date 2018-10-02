DESCRIPTION = "hello-world-mod tests the module.bbclass mechanism."
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

inherit sysmocom-module

PR = "r3"

SRC_URI = "http://www.kernel.org/pub/linux/kernel/projects/backports/stable/v3.18.1/backports-3.18.1-1.tar.xz \
           file://dont-export-clk_enable-twice.patch \
           file://scatterwalk-api-update.patch \
           file://defconfig \
          "
SRC_URI[md5sum] = "6cef5f2c800e12441d2cba9fa42b6a5b"
SRC_URI[sha256sum] = "ff3d4d5192c4d57d7415dfcd60e02ea4fa21e0de224ae0ce2b5b9f2e9c815783"

S = "${WORKDIR}/backports-3.18.1-1"


python __anonymous() {
    if d.getVar('DISTRO_VERSION', True)[0:3] == '1.5':
        d.setVar('KERNEL_BUILD_DIR', d.getVar('STAGING_KERNEL_DIR', True))
    else:
        d.setVar('KERNEL_BUILD_DIR', d.getVar('STAGING_KERNEL_BUILDDIR', True))
}

KERNEL_BUILD_DIR = "${@d.getVar('KERNEL_BUILD_DIR', True)}"

do_configure() {
	unset CFLAGS CPPFLAGS CXXFLAGS LDFLAGS CC
        cp ${WORKDIR}/defconfig ${S}/.config
        oe_runmake oldconfig KLIB_BUILD=${KERNEL_BUILD_DIR}
}

do_compile() {
        unset CFLAGS CPPFLAGS CXXFLAGS LDFLAGS
        oe_runmake KLIB_BUILD=${KERNEL_BUILD_DIR} \
                   CC="${KERNEL_CC}" LD="${KERNEL_LD}" \
                   AR="${KERNEL_AR}" \
                   ${MAKE_TARGETS}
}

do_install() {
        unset CFLAGS CPPFLAGS CXXFLAGS LDFLAGS
        oe_runmake DEPMOD=echo KLIB="${D}" \
                   KLIB_BUILD=${KERNEL_BUILD_DIR} \
                   CC="${KERNEL_CC}" LD="${KERNEL_LD}" \
                   install
}


KERNEL_MODULES_META_PACKAGE = "kernel-modules-backports"
KERNEL_MODULES_PACKAGE_PREFIX = "kernel-module-backports-%s"
