SYSMOCOM := "${@os.path.dirname(d.getVar('FILE', True))}"
FILESEXTRAPATHS_prepend := "${SYSMOCOM}/init-ifupdown-${PV}:${SYSMOCOM}/init-ifupdown-master:"
