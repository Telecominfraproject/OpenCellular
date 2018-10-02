THISDIR := "${@os.path.dirname(d.getVar('FILE', True))}"
FILESPATH =. "${@base_set_filespath(["${THISDIR}/files-master"], d)}:"

PRINC="4"
