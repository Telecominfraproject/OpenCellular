SUMMARY = "Control power outlet strip using USB"
LICENSE = "GPLv2+"
LIC_FILES_CHKSUM = "file://LICENCE;md5=393a5ca445f6965873eca0259a17f833"

SRC_URI = "${SOURCEFORGE_MIRROR}/${PN}/${PN}-${PV}.tar.gz \
	file://modernize.patch"

SRC_URI[md5sum] = "24693cae30d77c957f34cfb2c8159661"
SRC_URI[sha256sum] = "e9a99cc81ef0a93f3484e5093efd14d93cc967221fcd22c151f0bea32eb91da7"


EXTRA_OECONF = "--enable-webless"
DEPENDS = "libusb pkgconfig"

inherit autotools
