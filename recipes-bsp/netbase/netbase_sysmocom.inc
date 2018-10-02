SYSMOCOM := "${@os.path.dirname(d.getVar('FILE', True))}"
FILESEXTRAPATHS_prepend := "${SYSMOCOM}/${PN}-${PV}:${SYSMOCOM}/${PN}:"
PRINC = "21"


# bug in poky meta/classes/base.bbclass
PACKAGE_ARCH = "${MACHINE_ARCH}"
