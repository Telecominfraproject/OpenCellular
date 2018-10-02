DESCRIPTION = "slot initramfs"

PACKAGE_INSTALL = "initramfs-framework-base initramfs-module-debug initramfs-module-rauc-overlay initramfs-module-rauc-backup busybox base-passwd ${ROOTFS_BOOTSTRAP_INSTALL}"

# Do not pollute the initrd image with rootfs features
#IMAGE_FEATURES = ""

#export IMAGE_BASENAME = "core-image-minimal-initramfs"
IMAGE_LINGUAS = ""
FEED_URIS=""

LICENSE = "MIT"

IMAGE_FSTYPES = "cpio.xz"
# COMPRESS_CMD_xz = "xz -f -k -c ${XZ_COMPRESSION_LEVEL} ${XZ_THREADS} --check=${XZ_INTEGRITY_CHECK} ${IMAGE_NAME}.rootfs.${type} > ${IMAGE_NAME}.rootfs.${type}.xz"
XZ_COMPRESSION_LEVEL = "-e -9 --lzma2=dict=512KiB"
XZ_THREADS = "-T 0"
XZ_INTEGRITY_CHECK = "crc32"

inherit core-image

BAD_RECOMMENDATIONS += "busybox-syslog"
