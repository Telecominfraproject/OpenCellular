SUMMARY = "Modular initramfs system components for RAUC"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

PR = "r0"

SRC_URI = "file://install"

BOOTUSER = "${@d.getVar('BOOT_USER', True) or ""}"
BOOTPASSWD = "${@d.getVar('BOOT_PASSWD', True) or ""}"

do_install() {
    install -d ${D}/init.d

    if [ -n "${BOOTPASSWD}" ] && [ -n "${BOOTUSER}" ]; then
        sed -i ${WORKDIR}/install \
	-e "s/^\(.*BOOTPASSWD=\).*$/\1${BOOTPASSWD}/g" \
        -e "s/^\(.*BOOTUSER=\).*$/\1${BOOTUSER}/g"
    fi

    # install
    install -m 0755 ${WORKDIR}/install ${D}/init.d/10-install
}

PACKAGES = "initramfs-module-rauc-install"

SUMMARY_initramfs-module-rauc-install = "initramfs installer support"
RDEPENDS_initramfs-module-rauc-install = "initramfs-framework-base grub parted e2fsprogs-mke2fs"
FILES_initramfs-module-rauc-install = "/init.d/10-install"
