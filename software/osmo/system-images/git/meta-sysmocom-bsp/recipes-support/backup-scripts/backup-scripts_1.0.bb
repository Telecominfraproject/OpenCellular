SUMMARY = "Backup scripts"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/BSD;md5=3775480a712fc46a69647678acb234cb"

SRC_URI += "file://cbackup \
file://cbackup.sh \
file://mbackup \
file://mbackup.sh \
file://checkbk \
file://checkbk.service \
file://checkflashcfg \
file://checkflash.service \
"

S = "${WORKDIR}"

# set this variable to 0 to avoid blocking the boot process if flash configuration is not valid
# normally keep it to 1, in upper layers except if there is no way to know that a flash repair
# is needed from external server
export BLOCKOPTION  = "0"

inherit gitver-repo

REPODIR   = "${THISDIR}"
REPOFILE  = "backup-scripts_1.0.bb"
PR       := "r${REPOGITFN}"

REPODIR   = "${THISDIR}/files"
REPOFILE  = ""
PR       := "${PR}.${REPOGITFN}"

RDEPENDS_${PN} += "busybox cronie util-linux coreutils base-files"

do_install() {
     install -d ${D}${sysconfdir}
     install -d ${D}${sysconfdir}/systemd
     install -d ${D}${sysconfdir}/systemd/system
     install -d ${D}${sysconfdir}/systemd/system/multi-user.target.wants
     install -m 0755 -d ${D}${base_libdir}
     install -m 0755 -d ${D}${systemd_unitdir}
     install -m 0755 -d ${D}${systemd_unitdir}/system
     install -m 0644 ${S}/checkbk.service ${D}${systemd_unitdir}/system/checkbk.service
     ln -sf ${systemd_unitdir}/system/checkbk.service  ${D}${sysconfdir}/systemd/system/multi-user.target.wants/checkbk.service
     install -m 0644 ${S}/checkflash.service ${D}${systemd_unitdir}/system/checkflash.service
     ln -sf ${systemd_unitdir}/system/checkflash.service  ${D}${sysconfdir}/systemd/system/multi-user.target.wants/checkflash.service
     install -d ${D}${bindir}
     install -m 0755 ${S}/cbackup ${D}${bindir}/.cbackup
     install -m 0755 ${S}/mbackup ${D}${bindir}/.mbackup
     install -m 0755 ${S}/cbackup.sh ${D}${bindir}/cbackup
     install -m 0755 ${S}/mbackup.sh ${D}${bindir}/mbackup
     install -m 0755 ${S}/checkbk ${D}${bindir}/checkbk
     install -m 0755 ${S}/checkflashcfg ${D}${bindir}/checkflashcfg
     echo "BLOCKOPTION=${BLOCKOPTION}" > ${D}${sysconfdir}/bootoptions.conf
     chmod 755 ${D}${sysconfdir}/bootoptions.conf
}

pkg_postinst_${PN}_append() {
	echo "adding crontab"
	test -d $D/var/spool/cron || mkdir -p $D/var/spool/cron
	test -f /var/spool/cron/root && sed -i '/checkbk/d' $D/var/spool/cron/root
	echo "15 3 * * *    nice -n 15 ${bindir}/checkbk" >> $D/var/spool/cron/root
}

FILES_${PN} += "${bindir}/mbackup \
${bindir}/.mbackup \
${bindir}/cbackup \
${bindir}/.cbackup \
${bindir}/checkbk \
${bindir}/checkflashcfg \
${sysconfdir} \
${systemd_unitdir}/* \
            "
