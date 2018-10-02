SUMMARY = "Modular initramfs system components for RAUC"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

PR = "r2"

inherit allarch

SRC_URI = "file://overlay \
	   file://rescue \
	   file://mount_data \
	   file://restore_backup"

do_install() {
    install -d ${D}/init.d

    # overlay
    install -m 0755 ${WORKDIR}/overlay ${D}/init.d/20-overlay

    # rescue
    install -m 0755 ${WORKDIR}/rescue ${D}/init.d/10-rescue

    # backup
    install -m 0755 ${WORKDIR}/mount_data ${D}/init.d/25-mount_data
    install -m 0755 ${WORKDIR}/restore_backup ${D}/init.d/26-restore_backup
}

PACKAGES = "initramfs-module-rauc-overlay \
	    initramfs-module-rauc-rescue \
	    initramfs-module-rauc-backup"

SUMMARY_initramfs-module-rauc-overlay = "initramfs support for overlayfs (ubifs&squashfs)"
RDEPENDS_initramfs-module-rauc-overlay = "initramfs-framework-base"
FILES_initramfs-module-rauc-overlay = "/init.d/20-overlay"

SUMMARY_initramfs-module-rauc-rescue = "initramfs rescue mode support"
RDEPENDS_initramfs-module-rauc-rescue = "initramfs-framework-base"
FILES_initramfs-module-rauc-rescue = "/init.d/10-rescue"

SUMMARY_initramfs-module-rauc-backup = "initramfs backup restore support"
RDEPENDS_initramfs-module-rauc-backup = "initramfs-module-rauc-overlay"
FILES_initramfs-module-rauc-backup = "/init.d/25-mount_data /init.d/26-restore_backup"
