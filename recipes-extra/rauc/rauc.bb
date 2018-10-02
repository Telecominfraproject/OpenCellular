DESCRIPTION = "rauc update controller"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=4fbd65380cdd255951079008b364516c"
PR = "r1"

SRC_URI = "git://github.com/jluebbe/rauc.git;protocol=https \
	file://dev-ca.pem \
	file://system.conf \
	file://rauc-done.service \
	file://rauc-ubi.rules \
	file://system.conf "
PV = "0+git${SRCPV}"

S = "${WORKDIR}/git"

SRCREV = "011953fa6c824ca518cf0ea074ddeede3726bdcd"

DEPENDS = "curl openssl glib-2.0 glib-2.0-native"

# rauc is invoking these depending the machine
RDEPENDS_${PN}_append_sysmobts-v2 = " barebox-state"
RDEPENDS_${PN}_append_sysmocom-odu = " barebox-state"
RDEPENDS_${PN}_append_gsmk-owhw = " barebox-state"
RDEPENDS_${PN}_append_sysmocom-bsc = " grub"

FILES_${PN} += "/mnt/rauc"
FILES_${PN} += "${base_libdir}/udev/rules.d/*.rules"

inherit autotools pkgconfig gettext systemd

EXTRA_OECONF = "--disable-service"

do_install_append () {
	# Create rauc config dir
	mkdir -p ${D}${sysconfdir}/rauc

	# Create rauc default mountpoint
	mkdir -p ${D}/mnt/rauc

	# If a system.conf is provided in files, install it
	if [ -f ${WORKDIR}/system.conf ]; then
		install -m 644 ${WORKDIR}/system.conf ${D}${sysconfdir}/rauc/system.conf
	fi

	if [ -f ${WORKDIR}/dev-ca.pem ]; then
		mkdir -p ${D}${sysconfdir}/rauc
		install -m 644 ${WORKDIR}/dev-ca.pem ${D}${sysconfdir}/rauc/ca.pem
	fi

	# D-bus service
	install -d ${D}${sysconfdir}/dbus-1/system.d
	install -m 0644 ${S}/data/de.pengutronix.rauc.conf ${D}${sysconfdir}/dbus-1/system.d/

	# Systemd service
	install -d ${D}${systemd_system_unitdir}
	install -m 0644 ${WORKDIR}/rauc-done.service ${D}${systemd_system_unitdir}/
	sed -i -e 's!@BINDIR@!${bindir}!g' ${D}${systemd_system_unitdir}/*.service

	# udev rules
	install -d ${D}${base_libdir}/udev/rules.d
	install -m 0644 ${WORKDIR}/rauc-ubi.rules ${D}${base_libdir}/udev/rules.d/85-rauc-ubi.rules
}

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_SERVICE_${PN} = "rauc-done.service"
SYSTEMD_AUTO_ENABLE_${PN} = "enable"

