THISDIR := "${@os.path.dirname(d.getVar('FILE', True))}"
FILESPATH =. "${@base_set_filespath(["${THISDIR}/files"], d)}:"

PRINC="1"
