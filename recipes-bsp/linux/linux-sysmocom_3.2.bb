inherit kernel
require linux-sysmocom.inc

LINUX_VERSION ?= "3.2.69"
LINUX_VERSION_EXTENSION ?= "-sysmocom-${LINUX_KERNEL_TYPE}"

# ATTENTION: Update linux-backports PR on version change. In Dora the
# reverse dependency tracking for the kernel doesn't appear to work. So
# please bump the PR on version changes!
SRCREV = "d33286eda98596983abf9bd6420741fdfedd192f"
BRANCH = "sob-odu/linux-3.2.69"

PR = "r43"
PV = "${LINUX_VERSION}+git${SRCPV}"

SRC_URI = " \
	   git://git.sysmocom.de/sysmo-bts/linux.git;protocol=git;branch=${BRANCH} \
	   file://mISDN_loop.patch;patch=1 \
	   file://defconfig"
S = "${WORKDIR}/git"

COMPATIBLE_MACHINE = "(sysmocom-odu)"
EXTRA_OEMAKE += "KALLSYMS_EXTRA_PASS=1"

require linux-tools.inc

do_configure() {
	install -m 0644 ${WORKDIR}/defconfig ${B}/.config
	oe_runmake -C ${S} O=${B} oldconfig
}

# autoload defaults (alphabetically sorted)
module_autoload_leds-gpio = "leds-gpio"
module_autoload_mmc_block = "mmc_block"
KERNEL_MODULE_AUTOLOAD += "leds-gpio mmc_block"


# Legacy for 3.2 and LCR
module_autoload_mISDN_l1loop = "mISDN_l1loop"
module_autoload_mISDN_dsp = "mISDN_dsp"
module_conf_mISDN_l1loop = "options mISDN_l1loop pri=1 nchannel=20"
KERNEL_MODULE_PROBECONF_append = "mISDN_l1loop mISDN_dsp"
KERNEL_MODULE_AUTOLOAD += "mISDN_l1loop mISDN_dsp"

DEFAULT_PREFERENCE = "20"
