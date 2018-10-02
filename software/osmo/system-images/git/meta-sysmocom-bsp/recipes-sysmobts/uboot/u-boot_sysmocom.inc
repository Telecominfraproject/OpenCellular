SRC_URI = "git://git.sysmocom.de/sysmo-bts/u-boot;branch=tracking/upstream-next;protocol=git"
SRCREV = "64b07727fbb268bd19bacfed6711bdb060effcfa"
PV = "v2017.03+git${SRCPV}"
S = "${WORKDIR}/git"

PRINC = "13"

LIC_FILES_CHKSUM = "file://README;beginline=1;endline=22;md5=2687c5ebfd9cb284491c3204b726ea29"

UBOOT_MACHINE_sysmobts-v2 = "davinci_sysmobts_v2_config"
COMPATIBLE_MACHINE = "(sysmobts-v2)"

UBOOT_IMAGE = "u-boot-${MACHINE}-${PV}-${PR}"
UBOOT_SYMLINK = "u-boot-${MACHINE}"

DEPENDS += " bc-native "

do_install_append() {
    rm ${D}/boot/*
}

do_deploy_prepend () {
    install -d ${DEPLOYDIR}
    install ${B}/u-boot.bin ${DEPLOYDIR}/${UBOOT_IMAGE}.bin
    install ${B}/u-boot ${DEPLOYDIR}/${UBOOT_IMAGE}.elf

    cd ${DEPLOYDIR}
    rm -f ${UBOOT_SYMLINK}.bin
    rm -f ${UBOOT_SYMLINK}.elf
    ln -sf ${UBOOT_IMAGE}.bin ${UBOOT_SYMLINK}.bin
    ln -sf ${UBOOT_IMAGE}.elf ${UBOOT_SYMLINK}.elf

    if [ "x${SPL_UART_BINARY}" != "x" ]
    then
        install ${S}/spl/${SPL_UART_BINARY} ${DEPLOYDIR}/${SPL_UART_IMAGE}
        rm -f ${DEPLOYDIR}/${SPL_UART_BINARY} ${DEPLOYDIR}/${SPL_UART_SYMLINK}
        ln -sf ${SPL_UART_IMAGE} ${DEPLOYDIR}/${SPL_UART_BINARY}
        ln -sf ${SPL_UART_IMAGE} ${DEPLOYDIR}/${SPL_UART_SYMLINK}
    fi

    if [ "x${SPL_BINARY}" != "x" ]
    then
        install ${S}/${SPL_BINARY} ${DEPLOYDIR}/${SPL_IMAGE}
        rm -f ${DEPLOYDIR}/${SPL_BINARY} ${DEPLOYDIR}/${SPL_SYMLINK}
        ln -sf ${SPL_IMAGE} ${DEPLOYDIR}/${SPL_BINARY}
        ln -sf ${SPL_IMAGE} ${DEPLOYDIR}/${SPL_SYMLINK}
    fi

    # Stop the processing now
    return
}
