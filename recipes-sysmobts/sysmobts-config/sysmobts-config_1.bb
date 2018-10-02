DESCRIPTION = "Script to mangle the configuration"
SECTION = "core"
LICENSE = "CLOSED"

SRC_URI = "file://sysmobts-post-install-config"
PR = "r1"

do_install() {
	install -d ${D}${bindir}
	install -m 0755 ${WORKDIR}/sysmobts-post-install-config ${D}${bindir}/
}

pkg_postinst_${PN} () {
if test "x$D" != "x"; then
        exit 1
else
	echo "Going to run device specific post configuration"
	/usr/bin/sysmobts-post-install-config
fi
}

PACKAGE_ARCH = "all"
