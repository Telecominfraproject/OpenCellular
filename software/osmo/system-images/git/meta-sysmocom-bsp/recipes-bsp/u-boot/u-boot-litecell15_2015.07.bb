require u-boot.inc
require ${PN}-${PV}.inc

PROVIDES_litecell15 = " \
	u-boot \
 	virtual/bootloader \
"

DESCRIPTION = "u-boot bootloader for LC15 / sysmoBTS 2100"

REPODIR   = "${THISDIR}"
REPOFILE  = "u-boot-litecell15_2015.07.bb"
PR       := "${INC_PR}.${REPOGITFN}"

# set theses two variables to 1 to specify u-boot update requierement when the rootfs is updated 
export MLO_UPGRADE = "1"
export UBOOT_UPGRADE = "1"

SPL_BINARY = "MLO"
SPL_UART_BINARY = "u-boot-spl.bin"


