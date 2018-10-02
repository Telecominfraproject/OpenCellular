# I add another image type for the sysmoBTS family

UBI_VOLNAME ?= "${MACHINE}-rootfs"

IMAGE_CMD_ubi-sysmo () {
	echo \[kernel\] >> ubinize_sysmo.cfg
	echo mode=ubi >> ubinize_sysmo.cfg
	echo image=${DEPLOY_DIR_IMAGE}/uImage-${MACHINE}.bin >> ubinize_sysmo.cfg
	echo vol_id=0 >> ubinize_sysmo.cfg
	echo vol_type=static >> ubinize_sysmo.cfg
	echo vol_name=${MACHINE}-backup-kernel >> ubinize_sysmo.cfg
	echo \[ubifs\] >> ubinize_sysmo.cfg 
	echo mode=ubi >> ubinize_sysmo.cfg
	echo image=${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}.rootfs.ubifs-sysmo >> ubinize_sysmo.cfg 
	echo vol_id=1 >> ubinize_sysmo.cfg 
	echo vol_type=dynamic >> ubinize_sysmo.cfg 
	echo vol_name=${UBI_VOLNAME} >> ubinize_sysmo.cfg 
	echo vol_flags=autoresize >> ubinize_sysmo.cfg
	mkfs.ubifs -r ${IMAGE_ROOTFS} -o ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}.rootfs.ubifs-sysmo ${MKUBIFS_ARGS} && ubinize -o ${DEPLOY_DIR_IMAGE}/${IMAGE_NAME}.rootfs.ubi-sysmo ${UBINIZE_ARGS} ubinize_sysmo.cfg
}
