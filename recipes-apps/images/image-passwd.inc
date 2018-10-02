DEPENDS += "openssl-native"

rootpasswd = "${@d.getVar('ROOT_PASS', True) or ""}"

ROOTFS_POSTPROCESS_COMMAND += "set_root_passwd;"

set_root_passwd() {
   if [ -n "${rootpasswd}" ]; then
       HASHED_PASS=`openssl passwd -1 ${rootpasswd}`
       if [ -n "${HASHED_PASS}" ]; then
           sed -e "s%^root:[^:]*:%root:${HASHED_PASS}:%" \
               -i ${IMAGE_ROOTFS}/etc/shadow
           echo "Generated new root-passwd"
       fi
  fi
}
