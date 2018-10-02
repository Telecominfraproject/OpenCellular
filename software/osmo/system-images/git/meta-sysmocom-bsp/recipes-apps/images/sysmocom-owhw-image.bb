require recipes-apps/images/sysmocom-image.inc
require recipes-apps/images/image-passwd.inc
require recipes-apps/images/image-sshkey.inc

# have enough space for log files and db
IMAGE_INSTALL = "task-core-boot ${ROOTFS_PKGMANAGE} \
		 task-owhw-image task-sysmocom-debug \
		 task-sysmocom-tools"

# vim: tabstop=8 shiftwidth=8 noexpandtab

# create what the rauc slots expect...
link_kernel() {
    echo "Linking the current uImage to /kernel"
    OLD_PWD=$PWD

    cd ${IMAGE_ROOTFS}/
    ln ./boot/uImage-* ./kernel || true

    echo "Copying devicetree to /devicetree"
    cp "${DEPLOY_DIR_IMAGE}/uImage-am335x-gsmk-owhw.dtb" ./devicetree

    cd $OLD_PWD
}

IMAGE_PREPROCESS_COMMAND += "link_kernel; "
