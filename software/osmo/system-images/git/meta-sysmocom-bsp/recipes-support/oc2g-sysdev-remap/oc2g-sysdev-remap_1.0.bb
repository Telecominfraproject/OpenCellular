DESCRITOPN = "Remap all available system devices of OC-2G platform as symbolic links to easy to reach place in /var/oc2g/"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/BSD;md5=3775480a712fc46a69647678acb234cb"

inherit update-rc.d

SRC_URI = "file://oc2g-sysdev-remap \
	   file://oc2g-sysdev-remap.init \
	   file://oc2g-sysdev-remap.service"

S = "${WORKDIR}"

inherit gitver-repo

REPODIR   = "${THISDIR}"
REPOFILE  = "oc2g-sysdev-remap_1.0.bb"
PR       := "r${REPOGITFN}"

REPODIR   = "${THISDIR}/files"
REPOFILE  = ""
PR       := "${PR}.${REPOGITFN}"

RDEPENDS_${PN} += "backup-scripts"

do_install() {
	install -d ${D}${sysconfdir}
	install -d ${D}${sysconfdir}/systemd
	install -d ${D}${sysconfdir}/systemd/system
	install -d ${D}${sysconfdir}/systemd/system/multi-user.target.wants
	install -d ${D}${sysconfdir}/init.d
	install -m 0755 -d ${D}${base_libdir}
	install -m 0755 -d ${D}${systemd_unitdir}
	install -m 0755 -d ${D}${systemd_unitdir}/system
	install -d ${D}${bindir}
	install -d ${D}/var/volatile/oc2g
	ln -sf volatile/oc2g ${D}/var/oc2g
	install -m 0755 ${S}/oc2g-sysdev-remap ${D}${bindir}/oc2g-sysdev-remap
	install -m 0755 ${S}/oc2g-sysdev-remap.init ${D}${sysconfdir}/init.d/oc2g-sysdev-remap
	install -m 0644 ${S}/oc2g-sysdev-remap.service ${D}${systemd_unitdir}/system/oc2g-sysdev-remap.service
	ln -sf ${systemd_unitdir}/system/oc2g-sysdev-remap.service  ${D}${sysconfdir}/systemd/system/multi-user.target.wants/oc2g-sysdev-remap.service
}

FILES_${PN} += "${bindir} \
		${sysconfdir} \
		${systemd_unitdir} \
		${sysconfdir}/init.d \
		/var/oc2g"

INSANE_SKIP_${PN} = "arch"
INITSCRIPT_PACKAGES = "${PN}"
INITSCRIPT_NAME_${PN} = "oc2g-sysdev-remap"
INITSCRIPT_PARAMS_${PN} = "defaults 21 21"



