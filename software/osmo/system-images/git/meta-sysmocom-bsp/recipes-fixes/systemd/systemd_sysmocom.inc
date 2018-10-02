S_THISDIR := "${@os.path.dirname(d.getVar('FILE', True))}"
FILESPATH =. "${@base_set_filespath(["${S_THISDIR}/${PN}"], d)}:"

PRINC="26"

SRC_URI += "file://journald.conf \
            file://legacy.conf \
            file://0001-Feature-switch-ListenDatagram-from-run-systemd-journ.patch \
           "

SRC_URI_append_sysmobts-v2 = " file://system.conf"
SRC_URI_append_sysmocom-odu = " file://system.conf"
SRC_URI_append_gsmk-owhw = " file://system.conf"
SRC_URI_append_sysmobts2100 = " file://system.conf"
SRC_URI_append_oc2g = " file://system.conf"

# We want the journal on the ODU...
SRC_URI_append_sysmocom-idu = " file://system.conf"
SRC_URI_remove_sysmocom-idu = " file://0001-Feature-switch-ListenDatagram-from-run-systemd-journ.patch "

# Disable depedency on dbus
RDEPENDS_${PN}_remove = "dbus"


# TODO: disable systemd-logind as it needs dbus
SYSTEMD_DISABLED_NATIVE_SERVICES = " \
  systemd-logind \
"

# disable journald as it isn't efficient
SYSMOCOM_DISABLED_JOURNALD_SERVICES = " \
    systemd-journald.socket \
    systemd-journald.service \
    systemd-journal-flush.service \
"

SYSMOCOM_DISABLED_JOURNALD_SERVICES_sysmocom-idu = ""

pkg_postinst_${PN}_append () {
        cd $D${sysconfdir}/init.d

        echo "Disabling the following native services: "

        OPTS=""

        if [ -n "$D" ]; then
                OPTS="--root=$D"
        fi

        for i in ${SYSTEMD_DISABLED_NATIVE_SERVICES} ; do
                if [ \( -e $i -o $i.sh \) -a ! -e $D${sysconfdir}/systemd/system/$i.service ] ; then
                        echo -n "$i: " ; systemctl ${OPTS} mask $i.service
                fi
        done ; echo

        # disable journald because its using to much cpu under stress logging
        for i in ${SYSMOCOM_DISABLED_JOURNALD_SERVICES} ; do
            systemctl ${OPTS} mask $i
        done

}

do_install_append() {
        install -m 0644 ${WORKDIR}/journald.conf ${D}${sysconfdir}/systemd/
        install -m 0644 ${WORKDIR}/legacy.conf ${D}/usr/lib/tmpfiles.d/

        if [ -e ${WORKDIR}/system.conf ]; then
            install -m 0644 ${WORKDIR}/system.conf ${D}${sysconfdir}/systemd/
        fi
}
