THISDIR := "${@os.path.dirname(d.getVar('FILE', True))}"
FILESPATH =. "${@base_set_filespath(["${THISDIR}/${PN}-${PV}"], d)}:"

PRINC = "2"

do_install_append() {
	# This breaks autoloading of modules as they are loaded before depmod runs
	rm ${D}${datadir}/opkg/intercept/depmod
	rm ${D}${datadir}/opkg/intercept/update-modules
}
