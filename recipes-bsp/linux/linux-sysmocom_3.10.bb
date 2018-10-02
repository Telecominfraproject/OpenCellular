inherit kernel
require linux-sysmocom.inc

DEPENDS += "bc-native"

# ATTENTION: Update linux-backports PR on version change. In Dora the
# reverse dependency tracking for the kernel doesn't appear to work. So
# please bump the PR on version changes!
# at versions changes do not forget to update conf/machine/include/sysmobts.inc too
LINUX_VERSION ?= "3.10.84"
LINUX_VERSION_EXTENSION ?= "-sysmocom-${LINUX_KERNEL_TYPE}"

# Overrides for the sysmocom bts v2
BTS_FIRMWARE_NAME_sysmobts-v2 = "sysmobts-v2"

SRCREV = "60c9ebbd1a683e8691223042a12958c5dc661feb"

PR = "r43"
PV = "${LINUX_VERSION}+git${SRCPV}"

SRC_URI = "git://git.sysmocom.de/sysmo-bts/linux.git;protocol=git;branch=linux-3.10.84 \
	   file://defconfig"

S = "${WORKDIR}/git"

COMPATIBLE_MACHINE = "(sysmobts-v2|sysmocom-bsc)"
EXTRA_OEMAKE += "KALLSYMS_EXTRA_PASS=1"

require linux-tools.inc

do_configure() {
	install -m 0644 ${WORKDIR}/defconfig ${B}/.config
	oe_runmake -C ${S} O=${B} oldconfig
}

# autoload defaults (alphabetically sorted)
module_autoload_davinci_mmc = "davinci_mmc"
module_autoload_dspdl_dm644x = "dspdl_dm644x"
module_autoload_fpgadl_par = "fpgadl_par"
module_autoload_leds-gpio = "leds-gpio"
module_autoload_mmc_block = "mmc_block"
module_autoload_msgqueue = "msgqueue"
module_autoload_rtfifo = "rtfifo"

KERNEL_MODULE_PROBECONF_append = "davinci_mmc dspdl_dm644x fpgadl_par leds-gpio mmc_block msgqueue rtfifo"
KERNEL_MODULE_AUTOLOAD_append = "davinci_mmc dspdl_dm644x fpgadl_par leds-gpio mmc_block msgqueue rtfifo"

# module configs (alphabetically sorted)
module_conf_dspdl_dm644x = "options dspdl_dm644x fw_name=${BTS_FIRMWARE_NAME}.out debug=0"
module_conf_fpgadl_par = "options fpgadl_par fw_name=${BTS_FIRMWARE_NAME}.bit"
module_conf_msgqueue = "options msgqueue fw_name=${BTS_FIRMWARE_NAME}.out"
module_conf_rtfifo = "options rtfifo fw_name=${BTS_FIRMWARE_NAME}.out"

RDEPENDS_kernel-module-dspdl-dm644x += "sysmobts-firmware"
RDEPENDS_kernel-module-fpgadl-par += "sysmobts-firmware"
RDEPENDS_kernel-module-msgqueue += "sysmobts-firmware"
RDEPENDS_kernel-module-rtfifo += "sysmobts-firmware"

DEFAULT_PREFERENCE = "-1"
