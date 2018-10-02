inherit kernel
require linux-sysmocom.inc

DEPENDS += "bc-native"

# ATTENTION: Update linux-backports PR on version change. In Dora the
# reverse dependency tracking for the kernel doesn't appear to work. So
# please bump the PR on version changes!
# at versions changes do not forget to update conf/machine/include/sysmobts.inc too
LINUX_VERSION ?= "4.9.59"
LINUX_VERSION_EXTENSION ?= "-sysmocom-${LINUX_KERNEL_TYPE}"

# Overrides for the sysmocom bts v2
BTS_FIRMWARE_NAME_sysmobts-v2 = "sysmobts-v2"

SRCREV = "27afed74546b4fa5546aeea26128eae2dcdc7c1f"

PR = "r6"
PV = "${LINUX_VERSION}+git${SRCPV}"

SRC_URI = "git://git.sysmocom.de/sysmo-bts/linux.git;protocol=git;branch=tracking/linux-stable-4.9 \
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
KERNEL_MODULE_PROBECONF += "davinci_mmc dspdl_dm644x fpgadl_par leds-gpio mmc_block msgqueue rtfifo"
KERNEL_MODULE_AUTOLOAD += "davinci_mmc dspdl_dm644x fpgadl_par leds-gpio mmc_block msgqueue rtfifo"

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
