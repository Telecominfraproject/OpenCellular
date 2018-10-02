# we sed the udev automounter so it mounts all partions on the stick/sdcard readonly

change_udev_automounter_ro() {
        sed -i -e 's/\-t auto/\-t auto \-o ro/' ${IMAGE_ROOTFS}/etc/udev/scripts/mount.sh
}

bootuser = "${@d.getVar('BOOT_USER', True) or ""}"
bootpasswd = "${@d.getVar('BOOT_PASSWD', True) or ""}"

set_boot_passwd() {
   if [ -n "${bootpasswd}" ] && [ -n "${bootuser}" ]; then
       sed -e "s/^\(BOOTPASSWD=\)/\1${bootpasswd}/g" \
           -e "s/^\(BOOTUSER=\)/\1${bootuser}/g" \
           -i ${IMAGE_ROOTFS}/install.sh
       echo "Generated new grub-passwd"
  fi
}

ROOTFS_POSTPROCESS_COMMAND += "change_udev_automounter_ro ; set_boot_passwd; "
