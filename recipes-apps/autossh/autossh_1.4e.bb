DESCRIPTION = "autossh"
LICENSE = "MIT"


LIC_FILES_CHKSUM = "file://autossh.c;endline=22;md5=b2b08187a92c97723e1d882a9fe657ac"
SRC_URI = "http://www.harding.motd.ca/autossh/autossh-${PV}.tgz \
	file://020_use_destdir_makefile.diff \
	file://022_pass_ldflags.diff"

MIRRORS_append = "\n http://www.harding.motd.ca/autossh/.* https://downloads.sysmocom.de/public/mirror/source/ \n"

SRC_URI[md5sum] = "f86684b96e99d22b2e9d35dc63b0aa29"
SRC_URI[sha256sum] = "9e8e10a59d7619176f4b986e256f776097a364d1be012781ea52e08d04679156"

inherit autotools
B = "${S}"

PACKAGES += "${PN}-examples"

FILES_${PN}-examples = "${datadir}/examples/*"
