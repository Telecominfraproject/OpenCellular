
hostsshkeysfile = "${@d.getVar('SSH_KEYS_FILE', True) or ''}"

ROOTFS_POSTPROCESS_COMMAND += "copy_ssh_keys;"

copy_ssh_keys() {
    if [ -n "${hostsshkeysfile}" ]; then
        if [ -r "${hostsshkeysfile}" ]; then
            echo "Installing sshkeys from ${hostsshkeysfile}"
            mkdir -p ${IMAGE_ROOTFS}/home/root/.ssh/
            cat ${hostsshkeysfile} >> ${IMAGE_ROOTFS}/home/root/.ssh/authorized_keys
            chown root:root ${IMAGE_ROOTFS}/home/root/.ssh/authorized_keys
            chmod 0600 ${IMAGE_ROOTFS}/home/root/.ssh/authorized_keys
        else
            echo "Could not install sshkeys from ${hostsshkeysfile}"
        fi
    fi
}
