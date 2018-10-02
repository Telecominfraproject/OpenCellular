require ${COREBASE}/meta/recipes-bsp/u-boot/u-boot.inc

FILESEXTRAPATHS_prepend := "${THISDIR}/u-boot:"

INC_PR = "r2"

PACKAGE_ARCH = "${MACHINE_ARCH}"

PROVIDES += "u-boot"
PKG_${PN} = "u-boot"
PKG_${PN}-dev = "u-boot-dev"
PKG_${PN}-dbg = "u-boot-dbg"

S = "${WORKDIR}/git"

UBOOT_SUFFIX = "img"

#RDEPENDS_${PN} = "repair"

# SPL (Second Program Loader) to be loaded over UART
SPL_UART_BINARY ?= ""
SPL_UART_IMAGE ?= "${SPL_UART_BINARY}-${MACHINE}-${PV}-${PR}"
SPL_UART_SYMLINK ?= "${SPL_UART_BINARY}-${MACHINE}"
MLO_BIN ?= "MLO-${MACHINE}-${PV}-${PR}"

do_install_append () {
    if [ "x${SPL_UART_BINARY}" != "x" ]
    then
        install ${B}/spl/${SPL_UART_BINARY} ${D}/boot/${SPL_UART_IMAGE}
        ln -sf ${SPL_UART_IMAGE} ${D}/boot/${SPL_UART_BINARY}
    fi
    #FIXME: do we want/need this? replace iwth 'openssl enc -base64 -d'?
    #md5sum ${D}/boot/${MLO_BIN} | cut -d ' ' -f 1 | xxd -r -p >> ${D}/boot/${MLO_BIN}
    install -d ${D}${sysconfdir}
    echo "MLO_UPGRADE=${MLO_UPGRADE}" >> ${D}${sysconfdir}/mlo.conf
    echo "UBOOT_UPGRADE=${UBOOT_UPGRADE}" >> ${D}${sysconfdir}/uboot.conf
    chmod 755 ${D}${sysconfdir}/mlo.conf 
    chmod 755 ${D}${sysconfdir}/uboot.conf 
}

do_deploy_append () {
    cd ${DEPLOYDIR}
    if [ "x${SPL_UART_BINARY}" != "x" ]
    then
        install ${B}/spl/${SPL_UART_BINARY} ${DEPLOYDIR}/${SPL_UART_IMAGE}
        rm -f ${DEPLOYDIR}/${SPL_UART_BINARY} ${DEPLOYDIR}/${SPL_UART_SYMLINK}
        ln -sf ${SPL_UART_IMAGE} ${DEPLOYDIR}/${SPL_UART_BINARY}
        ln -sf ${SPL_UART_IMAGE} ${DEPLOYDIR}/${SPL_UART_SYMLINK}
    fi
    #FIXME: do we want/need this? replace iwth 'openssl enc -base64 -d'?
    #md5sum ${DEPLOYDIR}/${MLO_BIN} | cut -d ' ' -f 1 | xxd -r -p >> ${DEPLOYDIR}/${MLO_BIN}
}

pkg_postinst_${PN}_append() {
if [ x"$D" = "x" ]; then
    if [ -f /usr/bin/checkboot ]; then
        echo "Verify boot file possible update..."
        /usr/bin/checkboot -c -d
        __CHECK_RET=$?
        if test ${__CHECK_RET} -eq 100; then
            sleep 30s
            /usr/bin/checkboot -c -d
            __CHECK_RET=$?
        fi
        if test ${__CHECK_RET} -ne 0; then
            echo "Verify boot file possible update... error!"
        else
            echo "Verify boot file possible update... done!"
        fi
    fi
fi
}

FILES_${PN} += "${sysconfdir}/mlo.conf \
            ${sysconfdir}/uboot.conf \
            "
