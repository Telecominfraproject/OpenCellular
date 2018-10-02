DESCRIPTION = "UUCP is used to transfer mail, news and random files between systems which are not connected by more modern networks. The communication can be made via modems, direct (hard-wired) serial connections or via an IP connection."
HOMEPAGE = "http://www.airs.com/ian/uucp.html"
SECTION = "console/utils"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=94d55d512a9ba36caa9b7df079bae19f"
PR = "r3.5"

inherit autotools

SRC_URI = "ftp://ftp.gnu.org/pub/gnu/uucp/uucp-${PV}.tar.gz \
           file://uucp.logrotate \
           file://policy.patch"

do_configure() {
	libtoolize --force
	oe_runconf
}

do_install_append() {
	install -d ${D}${sysconfdir}/logrotate.d
	install -m 0644 ${WORKDIR}/uucp.logrotate ${D}${sysconfdir}/logrotate.d/uucp
}

EXTRA_OECONF = "--with-newconfigdir=/etc/uucp"

pkg_postinst_${PN} () {
	if [ "x$D" != "x" ] ; then
		exit 1
	fi

	# Create the UUCP directory if it does not exist
	if [ ! -e /var/spool/uucp ] ; then
        	mkdir -m 0770 /var/spool/uucp
	fi

	chown uucp:uucp /var/spool/uucp
	chmod 770 /var/spool/uucp
}

RDEPENDS_${PN} = "logrotate"
CONFFILES_${PN} = "${sysconfdir}/logrotate.d/uucp"

PACKAGES =+ "cu"
FILES_cu = "${bindir}/cu /var/spool"

SRC_URI[md5sum] = "64c54d43787339a7cced48390eb3e1d0"
SRC_URI[sha256sum] = "060c15bfba6cfd1171ad81f782789032113e199a5aded8f8e0c1c5bd1385b62c"
