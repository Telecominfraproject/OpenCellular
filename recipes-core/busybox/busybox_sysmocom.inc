SYSMOCOM := "${@os.path.dirname(d.getVar('FILE', True))}"
FILESEXTRAPATHS_prepend := "${SYSMOCOM}/busybox-${SYSMOCOM_ORIG_PV}:${SYSMOCOM}/files:"
PRINC = "32"

SRC_URI += "file://udhcpc-ntp.sh \
            file://udhcpc-ntp.default \
            file://udhcpc.default \
           "

PACKAGES =+ "${PN}-udhcpc-ntp"

FILES_${PN}-udhcpc += "${sysconfdir}/default/udhcpc"

FILES_${PN}-udhcpc-ntp = "${sysconfdir}/udhcpc.d/60ntp \
                          ${sysconfdir}/default/udhcpc-ntp \
                         "

CONFFILES_${PN}-udhcpc += "${sysconfdir}/default/udhcpc"
CONFFILES_${PN}-udhcpc-ntp = "${sysconfdir}/default/udhcpc-ntp"

RDEPENDS_${PN}-udhcpc-ntp = "${PN}-udhcpc"

do_install_append() {
	install -m 0755 ${WORKDIR}/udhcpc-ntp.sh ${D}/${sysconfdir}/udhcpc.d/60ntp
	install -d ${D}/${sysconfdir}/default
	install -m 0644 ${WORKDIR}/udhcpc-ntp.default ${D}/${sysconfdir}/default/udhcpc-ntp
        install -m 0644 ${WORKDIR}/udhcpc.default ${D}/${sysconfdir}/default/udhcpc
}
