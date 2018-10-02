DESCRIPTION = "ubi with rescue slot"

LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COREBASE}/LICENSE;md5=4d92cd373abda3937c2bc47fbc49d690 \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"

PACKAGES = ""
PACKAGE_ARCH = "${MACHINE_ARCH}"

do_fetch[cleandirs] = "${S}"
do_unpack[noexec] = "1"
do_patch[noexec] = "1"
do_configure[noexec] = "1"
do_compile[noexec] = "1"
do_install[noexec] = "1"
do_populate_sysroot[noexec] = "1"
do_package[noexec] = "1"
do_packagedata[noexec] = "1"
do_package_write_ipk[noexec] = "1"
do_package_write_deb[noexec] = "1"
do_package_write_rpm[noexec] = "1"

do_fetch[depends] += "virtual/kernel:do_deploy image-rauc-rescue-initramfs:do_image_complete"
do_deploy[depends] += "mtd-utils-native:do_populate_sysroot"

S = "${WORKDIR}"

do_fetch() {
	mkdir -p "${S}/fs"

	cp "${DEPLOY_DIR_IMAGE}/${KERNEL_IMAGETYPE}-${MACHINE}.bin" "${S}/fs/kernel"
	cp "${DEPLOY_DIR_IMAGE}/image-rauc-rescue-initramfs-${MACHINE}.cpio.xz" "${S}/fs/initramfs"
}

do_fetch_append_gsmk-owhw() {
	cp "${DEPLOY_DIR_IMAGE}/uImage-am335x-gsmk-owhw.dtb" "${S}/fs/devicetree"
}

IMAGE_ROOTFS = "${S}/fs"
IMAGE_NAME = "${PN}-${MACHINE}-${DATETIME}"
# Don't include the DATETIME variable in the sstate package sigantures
IMAGE_NAME[vardepsexclude] = "DATETIME"
IMAGE_LINK_NAME = "${PN}-${MACHINE}"

do_deploy() {
	echo \[rescue\] > ubinize.cfg
	echo mode=ubi >> ubinize.cfg
	echo image=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}.rescue.ubifs >> ubinize.cfg
	echo vol_id=0 >> ubinize.cfg
	echo vol_size=14MiB >> ubinize.cfg
	echo vol_type=dynamic >> ubinize.cfg
	echo vol_name=rescue >> ubinize.cfg
	echo \[system0\] >> ubinize.cfg
	echo mode=ubi >> ubinize.cfg
	echo vol_id=1 >> ubinize.cfg
	echo vol_size=35MiB >> ubinize.cfg
	echo vol_type=dynamic >> ubinize.cfg
	echo vol_name=system0 >> ubinize.cfg
	echo \[system1\] >> ubinize.cfg
	echo mode=ubi >> ubinize.cfg
	echo vol_id=2 >> ubinize.cfg
	echo vol_size=35MiB >> ubinize.cfg
	echo vol_type=dynamic >> ubinize.cfg
	echo vol_name=system1 >> ubinize.cfg
	echo \[data\] >> ubinize.cfg
	echo mode=ubi >> ubinize.cfg
	echo vol_id=3 >> ubinize.cfg
	echo vol_size=16MiB >> ubinize.cfg
	echo vol_type=dynamic >> ubinize.cfg
	echo vol_name=data >> ubinize.cfg
	echo vol_flags=autoresize >> ubinize.cfg

	mkfs.ubifs --squash-uids -r ${IMAGE_ROOTFS} -o ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}.rescue.ubifs ${MKUBIFS_ARGS}
	ubinize -o ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}.ubi ${UBINIZE_ARGS} ubinize.cfg
	ln -sf ${IMAGE_NAME}.ubi ${DEPLOY_DIR_IMAGE}/${IMAGE_LINK_NAME}.ubi
}

addtask deploy after do_fetch before do_build
