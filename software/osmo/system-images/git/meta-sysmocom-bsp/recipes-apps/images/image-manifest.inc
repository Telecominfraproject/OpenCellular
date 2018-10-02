create_manifest() {
    printf "Build_Date: %s\n" "`date -R`" > ${IMAGE_ROOTFS}/etc/sysmocom.manifest
    printf "UUID: %s\n" "`uuidgen -r`" >> ${IMAGE_ROOTFS}/etc/sysmocom.manifest
    printf "Machine: %s\n" "${MACHINE}" >> ${IMAGE_ROOTFS}/etc/sysmocom.manifest
    printf "Image: %s\n" "${IMAGE_BASENAME}" >> ${IMAGE_ROOTFS}/etc/sysmocom.manifest
    printf "Distro: %s\n" "${DISTRO}" >> ${IMAGE_ROOTFS}/etc/sysmocom.manifest
    printf "Distro_Version: %s\n" "${DISTRO_VERSION}" >> ${IMAGE_ROOTFS}/etc/sysmocom.manifest
}

IMAGE_PREPROCESS_COMMAND += "create_manifest; "
