DEPENDS = "${MACHINE_EXTRA_RDEPENDS} util-linux-native"
IMAGE_INSTALL = "task-core-boot ${ROOTFS_PKGMANAGE} packagegroup-osmocom task-sysmocom-debug task-sysmocom-tools ${MACHINE_EXTRA_RDEPENDS} "
IMAGE_LINGUAS = " "
IMAGE_FEATURES += " package-management "
LICENSE = "MIT"

inherit core-image
require recipes-apps/images/image-manifest.inc

IMAGE_ROOTFS_SIZE = "32768"

IMAGE_FSTYPES += "squashfs"
IMAGE_CMD_squashfs += "-e boot"

link_uimage() {
    echo "Linking the current uImage to /boot/uImage"
    OLD_PWD=$PWD
    cd ${IMAGE_ROOTFS}/boot
    ln -s uImage-* uImage || true
    cd $OLD_PWD
}

IMAGE_PREPROCESS_COMMAND += "link_uimage; "
