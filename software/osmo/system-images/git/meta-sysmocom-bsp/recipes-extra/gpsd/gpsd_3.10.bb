SUMMARY = "A TCP/IP Daemon simplifying the communication with GPS devices"
SECTION = "console/network"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://COPYING;md5=d217a23f408e91c94359447735bc1800"
DEPENDS = "ncurses python"
PROVIDES = "virtual/gpsd"

PR = "r3.19"

SRC_URI = "http://download.savannah.gnu.org/releases/${PN}/${P}.tar.gz \
  file://0002-SConstruct-respect-sysroot-also-in-SPLINTOPTS.patch \
  file://0001-SConstruct-disable-html-and-man-docs-building-becaus.patch \
  file://gpsd-3.3-ldflags.patch \
  file://no-rpath-please.patch \
  file://gpsd-tsip-pps.patch \
  file://leave-argv-untouched.patch \
  file://0001-gps2udp-Add-a-label-timestamp-and-mac-address-to-eac.patch \
  file://gpsd-default \
  file://gpsd \
  file://60-gpsd.rules \
    file://gpsd.service \
"

SRC_URI[md5sum] = "fc5b03aae38b9b5b6880b31924d0ace3"
SRC_URI[sha256sum] = "706fc2c1cf3dfbf87c941f543381bccc9c4dc9f8240eec407dcbf2f70b854320"

inherit scons update-rc.d systemd

INITSCRIPT_NAME = "gpsd"
INITSCRIPT_PARAMS = "defaults 35"

SYSTEMD_OESCONS = "${@bb.utils.contains('DISTRO_FEATURES', 'systemd', 'true', 'false',d)}"

export STAGING_INCDIR
export STAGING_LIBDIR
export LINKFLAGS="${TARGET_LDFLAGS}"
export SHLINKFLAGS="${TARGET_LDFLAGS}"

EXTRA_OESCONS = " \
  sysroot=${STAGING_DIR_TARGET} \
  libQgpsmm='false' \
  debug='true' \
  strip='false' \
  ashtec='false' \
  bluez='false' \
  dbus_export='false' \
  cheapfloats='false' \
  earthmate='false' \
  evermore='false' \
  fury='false' \
  fv18='false' \
  garmin='false' \
  garmintxt='false' \
  geostar='false' \
  itrax='false' \
  libQgpsmm='false' \
  mtk3301='false' \
  navcom='false' \
  oncore='false' \
  python='false' \
  sirf='false' \
  tnt='false' \
  trip='false' \
  tripmate='false' \
  usb='false' \
  chrpath='false' \
    systemd='${SYSTEMD_OESCONS}' \
    libdir='${libdir}' \
    ${PACKAGECONFIG_CONFARGS} \
"
# this cannot be used, because then chrpath is not found and only static lib is built
# target=${HOST_SYS}

do_compile_prepend() {
    export PKG_CONFIG_PATH="${PKG_CONFIG_PATH}"
    export PKG_CONFIG="PKG_CONFIG_SYSROOT_DIR=\"${PKG_CONFIG_SYSROOT_DIR}\" pkg-config"
    export STAGING_PREFIX="${STAGING_DIR_HOST}/${prefix}"

    export BUILD_SYS="${BUILD_SYS}"
    export HOST_SYS="${HOST_SYS}"
}

do_install() {
    export PKG_CONFIG_PATH="${PKG_CONFIG_PATH}"
    export PKG_CONFIG="PKG_CONFIG_SYSROOT_DIR=\"${PKG_CONFIG_SYSROOT_DIR}\" pkg-config"
    export STAGING_PREFIX="${STAGING_DIR_HOST}/${prefix}"

    export BUILD_SYS="${BUILD_SYS}"
    export HOST_SYS="${HOST_SYS}"

    export DESTDIR="${D}"
    # prefix is used for RPATH and DESTDIR/prefix for instalation
    ${STAGING_BINDIR_NATIVE}/scons prefix=${prefix} install ${EXTRA_OESCONS}|| \
      bbfatal "scons install execution failed."
}

do_install_append() {
    install -d ${D}/${sysconfdir}/init.d
    install -m 0755 ${WORKDIR}/gpsd ${D}/${sysconfdir}/init.d/
    install -d ${D}/${sysconfdir}/default
    install -m 0644 ${WORKDIR}/gpsd-default ${D}/${sysconfdir}/default/gpsd.default

    #support for udev
    install -d ${D}/${sysconfdir}/udev/rules.d
    install -m 0644 ${WORKDIR}/60-gpsd.rules ${D}/${sysconfdir}/udev/rules.d
    install -d ${D}${base_libdir}/udev/
    install -m 0755 ${S}/gpsd.hotplug ${D}${base_libdir}/udev/

    #support for systemd
    install -d ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/${BPN}.service ${D}${systemd_system_unitdir}/${BPN}.service
    install -m 0644 ${S}/systemd/${BPN}.socket ${D}${systemd_system_unitdir}/${BPN}.socket
}

pkg_postinst_${PN}-conf() {
    update-alternatives --install ${sysconfdir}/default/gpsd gpsd-defaults ${sysconfdir}/default/gpsd.default 10
}

pkg_postrm_${PN}-conf() {
    update-alternatives --remove gpsd-defaults ${sysconfdir}/default/gpsd.default
}

PACKAGES =+ "libgps libgpsd gpsd-udev gpsd-conf gpsd-gpsctl gps-utils"

RDEPENDS_${PN} = "gpsd-gpsctl"
RRECOMMENDS_${PN} = "gpsd-conf gpsd-udev gpsd-machine-conf"

SUMMARY_gpsd-udev = "udev relevant files to use gpsd hotplugging"
FILES_gpsd-udev = "${base_libdir}/udev ${sysconfdir}/udev/*"
RDEPENDS_gpsd-udev += "udev gpsd-conf"

SUMMARY_libgpsd = "C service library used for communicating with gpsd"
FILES_libgpsd = "${libdir}/libgpsd.so.*"

SUMMARY_libgps = "C service library used for communicating with gpsd"
FILES_libgps = "${libdir}/libgps.so.*"

SUMMARY_gpsd-conf = "gpsd configuration files and init scripts"
FILES_gpsd-conf = "${sysconfdir}"
FILES_gpsd-conf_append_sysmocom-idu = " ${systemd_system_unitdir}/ "
CONFFILES_gpsd-conf = "${sysconfdir}/default/gpsd.default"

SUMMARY_gpsd-gpsctl = "Tool for tweaking GPS modes"
FILES_gpsd-gpsctl = "${bindir}/gpsctl"

SUMMARY_gps-utils = "Utils used for simulating, monitoring,... a GPS"
FILES_gps-utils = "${bindir}/*"

RPROVIDES_${PN} += "${PN}-systemd"
RREPLACES_${PN} += "${PN}-systemd"
RCONFLICTS_${PN} += "${PN}-systemd"
SYSTEMD_SERVICE_${PN} = "${PN}.socket"
