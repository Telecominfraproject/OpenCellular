IMAGE_INSTALL = "task-core-boot packagegroup-osmocom task-sysmocom-tools"
IMAGE_LINGUAS = " "
LICENSE = "MIT"

inherit core-image
require recipes-apps/images/image-manifest.inc

IMAGE_ROOTFS_SIZE = "8192"

# remove not needed ipkg informations
ROOTFS_POSTPROCESS_COMMAND += "remove_packaging_data_files ; "
