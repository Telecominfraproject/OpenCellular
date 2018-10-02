DESCRIPTION = "sysmocom Kernel"
SECTION = "kernel"
LICENSE = "GPLv2"

LIC_FILES_CHKSUM = "file://COPYING;md5=d7810fab7487fb0aad327b76f1be7cd7"

# Set this to 'preempt_rt' in the local.conf if you want a real time kernel
LINUX_KERNEL_TYPE ?= "standard"

FILES_${PN} = ""

# Remove when using a Poky/OE-Core with that includes e0bf758982843ec1981b74410616b3492c599d06
pkg_postinst_kernel () {
       update-alternatives --install /${KERNEL_IMAGEDEST}/${KERNEL_IMAGETYPE} ${KERNEL_IMAGETYPE} ${KERNEL_IMAGETYPE}-${KERNEL_VERSION} ${KERNEL_PRIORITY} || true
}

pkg_postrm_kernel () {
       update-alternatives --remove ${KERNEL_IMAGETYPE} ${KERNEL_IMAGETYPE}-${KERNEL_VERSION} || true
}
