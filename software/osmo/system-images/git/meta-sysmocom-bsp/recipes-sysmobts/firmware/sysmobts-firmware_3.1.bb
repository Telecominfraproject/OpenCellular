COMPATIBLE_MACHINE = "sysmobts-v2"
FIRMWARE-VERSION = "superfemto_v${PV}"
PR = "r6.${INC_PR}"

require ${PN}.inc

S2 = "${WORKDIR}/sysmobts-firmware-superfemto_v3.0.1pre"

# Currently there is no common Firmware for RevC and RevD Hardware and we
# need to handle this differently for now.

SRC_URI = "file://sysmobts-firmware-${FIRMWARE-VERSION}.tar.bz2 \
	   file://sysmobts-firmware-superfemto_v3.0.1pre.tar.bz2"


do_install() {
	install -d ${D}/lib/firmware/

	# Install the firmware for revD
	install -m 0666 ${S}/Image/Dsp/superfemto.out ${D}/lib/firmware/sysmobts-v2-revd.out
	install -m 0666 ${S}/Image/Fpga/superfemto.bit ${D}/lib/firmware/sysmobts-v2-revd.bit

	# Install the firmware for revC
	install -m 0666 ${S2}/Image/Dsp/superfemto.out ${D}/lib/firmware/sysmobts-v2-revc.out
	install -m 0666 ${S2}/Image/Fpga/superfemto.bit ${D}/lib/firmware/sysmobts-v2-revc.bit
}

# Change a symlink depending on revC or revD. We are using the EEPROM size
# as an indicator for revC and revD
pkg_postinst_${PN} () {
# Building the rootfs?
if [ "x$D" != "x" ]; then
	exit 1
fi

EEPROM_SIZE=`stat -c '%s' /sys/bus/i2c/devices/1-0050/eeprom`
if [ $EEPROM_SIZE -eq 256 ]; then
	cd /lib/firmware
	ln -fs sysmobts-v2-revc.out sysmobts-v2.out
	ln -fs sysmobts-v2-revc.bit sysmobts-v2.bit
else
	cd /lib/firmware
	ln -fs sysmobts-v2-revd.out sysmobts-v2.out
	ln -fs sysmobts-v2-revd.bit sysmobts-v2.bit
fi
}

FILES_${PN} = "/lib/firmware/sysmobts-v2*"
