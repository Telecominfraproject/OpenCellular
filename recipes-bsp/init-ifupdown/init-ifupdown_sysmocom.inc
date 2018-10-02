SYSMOCOM := "${@os.path.dirname(d.getVar('FILE', True))}"
FILESEXTRAPATHS_prepend := "${SYSMOCOM}/init-ifupdown-${PV}:${SYSMOCOM}/init-ifupdown:"
PRINC = "13"

# systemd does not look at /etc/rcS.d for the script. Make sure
# there is a symlink available
INITSCRIPT_PARAMS = "start 40 S 2 . stop 40 0 6 1 ."
