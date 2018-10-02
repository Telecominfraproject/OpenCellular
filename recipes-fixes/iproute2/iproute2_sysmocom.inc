THISDIR := "${@os.path.dirname(d.getVar('FILE', True))}"
FILESPATH =. "${@base_set_filespath(["${THISDIR}/files/${PN}-${PV}"], d)}:${@base_set_filespath(["${THISDIR}/files"], d)}:"

PRINC="2"

# arpd can not be build because the db_185.h header from
# berkley db is not installed
SRC_URI += "file://disable_arpd_in_misc.patch"

EXTRA_OEMAKE = "CC='${CC}' KERNEL_INCLUDE=${STAGING_INCDIR} DOCDIR=${docdir}/iproute2 SUBDIRS='lib tc ip misc' SBINDIR='${base_sbindir}' LIBDIR='${libdir}'"

PACKAGES =+ "${PN}-misc"

FILES_${PN}-misc += "${base_sbindir}/ss \
                    ${base_sbindir}/nstat \
                    ${base_sbindir}/ifstat \
                    ${base_sbindir}/rtacct \
                    ${base_sbindir}/lnstat \
                   "
