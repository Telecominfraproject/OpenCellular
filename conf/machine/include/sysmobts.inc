TARGET_ARCH = "arm"

PREFERRED_PROVIDER_virtual/kernel = "linux-sysmocom"
PREFERRED_VERSION_linux-sysmocom = "${@dict([('1.5', '3.10.84+git%')]).get(d.getVar('DISTRO_VERSION', True)[0:3],'4.9.59+git%')}"
PREFERRED_PROVIDERS += "virtual/${TARGET_PREFIX}depmod:module-init-tools-cross"

PREFERRED_VERSION_u-boot = "git"

KERNEL_IMAGETYPE = "uImage"

UBOOT_ENTRYPOINT = "0x80008000"
UBOOT_LOADADDRESS = "0x80008000"

SERIAL_CONSOLE ?= "115200 ttyS0"
EXTRA_IMAGECMD_jffs2 = "--pad --little-endian --eraseblock=0x20000 -n"

#ROOT_FLASH_SIZE = "29"

MACHINE_FEATURES = "kernel26 serial"

MACHINE_ESSENTIAL_EXTRA_RDEPENDS = "\
	${@['', 'busybox-ifplugd'][d.getVar('DISTRO_VERSION', True)[0:3] == '1.5']} \
	${@['watchdog', ''][d.getVar('VIRTUAL-RUNTIME_init_manager', True) == 'systemd']} \
	kernel \
	kernel-module-dspdl \
	kernel-module-dspdl-dm644x \
	kernel-module-fpgadl \
	kernel-module-fpgadl-par \
	kernel-module-leds-gpio \
	kernel-module-msgqueue \
	kernel-module-nls-ascii \
	kernel-module-nls-utf8 \
	kernel-module-rtfifo "

MACHINE_EXTRA_RDEPENDS = "\
	task-sysmocom-bts \
	sysmobts-firmware \
	${@['watchdog', ''][d.getVar('VIRTUAL-RUNTIME_init_manager', True) == 'systemd']} \
	"
#MACHINE_EXTRA_RRECOMMENDS = "dsplink-module"

require conf/machine/include/tune-arm926ejs.inc
require conf/machine/include/dm6446.inc

# we tune for armv5te but it ends up as armv5e on pyro and probably earlier. Help it.
ARMPKGSFX_THUMB="t"
