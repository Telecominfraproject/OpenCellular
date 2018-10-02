DESCRIPTION = "rescue initramfs"

PACKAGE_INSTALL = "task-core-boot ${ROOTFS_PKGMANAGE} rauc dropbear"

IMAGE_LINGUAS = ""
FEED_URIS=""

LICENSE = "MIT"

# cpio config
XZ_COMPRESSION_LEVEL = "-e -9 --lzma2=dict=512KiB"
XZ_THREADS = "-T 0"
XZ_INTEGRITY_CHECK = "crc32"

IMAGE_FSTYPES = "cpio.xz"

BAD_RECOMMENDATIONS_append = " busybox-syslog kbd kbd-consolefonts kbd-keymaps"
BAD_RECOMMENDATIONS_append_sysmobts-v2 = " e2fsprogs-e2fsck"
BAD_RECOMMENDATIONS_append_sysmocom-odu = " e2fsprogs-e2fsck"
BAD_RECOMMENDATIONS_append_gsmk-owhw = " e2fsprogs-e2fsck"

inherit core-image
require recipes-apps/images/image-manifest.inc

shrink_rescue() {
    # The kernel should not be in the initramfs
    rm -rf ${IMAGE_ROOTFS}/boot

    # In case of the sysmoBTS.. remove files
    rm -rf ${IMAGE_ROOTFS}/lib/firmware/sysmobts*
    rm -rf ${IMAGE_ROOTFS}/lib/modules/*/kernel/drivers/staging/sysmobts/msgqueue.ko
    rm -rf ${IMAGE_ROOTFS}/lib/modules/*/kernel/drivers/misc/fpgadl.ko
    rm -rf ${IMAGE_ROOTFS}/lib/modules/*/kernel/drivers/staging/sysmobts/rtfifo.ko
    rm -rf ${IMAGE_ROOTFS}/lib/modules/*/kernel/drivers/misc/dspdl.ko
    rm -rf ${IMAGE_ROOTFS}/lib/modules/*/kernel/drivers/misc/dspdl_dm644x.ko
    rm -rf ${IMAGE_ROOTFS}/lib/modules/*/kernel/drivers/misc/fpgadl_par.ko

    # Who cares about udev?
    rm -rf ${IMAGE_ROOTFS}/lib/udev/hwdb.d/*
    rm -rf ${IMAGE_ROOTFS}/etc/udev/hwdb.bin
}

IMAGE_PREPROCESS_COMMAND += "shrink_rescue; "
