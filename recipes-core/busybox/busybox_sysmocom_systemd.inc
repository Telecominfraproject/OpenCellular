# Make busybox work nicely with systemd
SYSMOCOM_D := "${@os.path.dirname(d.getVar('FILE', True))}"
FILESEXTRAPATHS_prepend := "${SYSMOCOM}/${PN}-systemd:${SYSMOCOM_D}/${PN}:"
PRINC := "${@int(PRINC) + 3}"
