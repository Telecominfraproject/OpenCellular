SYSMOCOM := "${@os.path.dirname(d.getVar('FILE', True))}"
FILESEXTRAPATHS_prepend := "${SYSMOCOM}/files:"

PRINC = "5"

RDEPENDS_${PN} += "e2fsprogs-e2fsck e2fsprogs-resize2fs"
