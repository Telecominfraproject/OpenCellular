PACKAGES_prepend = "${PN}-rtl-nic "

RDEPENDS_${PN}-rtl-nic += "${PN}-rtl-license"

FILES_${PN}-rtl-nic = "/lib/firmware/rtl_nic/*.fw"
