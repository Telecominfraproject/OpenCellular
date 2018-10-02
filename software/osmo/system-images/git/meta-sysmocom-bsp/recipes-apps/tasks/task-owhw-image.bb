DESCRIPTION = "Task for OWHW hardware"
LICENSE = "MIT"
LIC_FILES_CHKSUM = " \
		    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
ALLOW_EMPTY_${PN} = "1"
PR = "r2"

RDEPENDS_${PN} = "usbutils openvpn gpsd gps-utils dropbear \
	wget ntp ca-cacert-rootcert early-date i2c-tools \
	wireless-tools iw crda gpsdate \
	kernel-module-cfg80211 \
	kernel-module-mac80211 \
	kernel-module-rt2x00lib \
	kernel-module-rt2x00usb \
	kernel-module-rt2800lib \
	kernel-module-rt2800usb \
	linux-firmware-ralink \
	procps iputils \
"

# vim: tabstop=8 shiftwidth=8 noexpandtab
