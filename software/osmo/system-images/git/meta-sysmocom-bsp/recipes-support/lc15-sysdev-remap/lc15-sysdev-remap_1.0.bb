DESCRITOPN = "Remap all available system devices of Litecell15 platform as symbolic links to easy to reach place in /var/lc15/"
LICENSE = "BSD"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/files/common-licenses/BSD;md5=3775480a712fc46a69647678acb234cb"

SRC_URI = "file://lc15-sysdev-remap \
	   file://mnt-rom-factory.automount \
	   file://mnt-rom-factory.mount \
	   file://mnt-rom-user.automount \
	   file://mnt-rom-user.mount \
	   file://mnt-storage.automount \
	   file://mnt-storage.mount \
	   file://lc15-sysdev-remap.service"

S = "${WORKDIR}"

inherit gitver-repo systemd

REPODIR   = "${THISDIR}"
REPOFILE  = "lc15-sysdev-remap_1.0.bb"
PR       := "r${REPOGITFN}"

REPODIR   = "${THISDIR}/files"
REPOFILE  = "."
PR       := "${PR}.${REPOGITFN}"

SYSTEMD_PACKAGES = "${PN}"
SYSTEMD_AUTO_ENABLE_${PN}="enable"
SYSTEMD_SERVICE_${PN} = "lc15-sysdev-remap.service mnt-rom-factory.automount mnt-rom-factory.mount mnt-rom-user.automount mnt-rom-user.mount mnt-storage.automount mnt-storage.mount"

do_install() {
	install -d ${D}${bindir}
	install -d ${D}/${systemd_system_unitdir}
	install -m 0755 ${S}/lc15-sysdev-remap ${D}${bindir}/lc15-sysdev-remap
	install -m 0644 ${S}/lc15-sysdev-remap.service ${D}${systemd_system_unitdir}/
	install -m 0644 ${S}/mnt-rom-factory.automount ${D}${systemd_system_unitdir}/
	install -m 0644 ${S}/mnt-rom-factory.mount ${D}${systemd_system_unitdir}/
	install -m 0644 ${S}/mnt-rom-user.automount ${D}${systemd_system_unitdir}/
	install -m 0644 ${S}/mnt-rom-user.mount ${D}${systemd_system_unitdir}/
	install -m 0644 ${S}/mnt-storage.automount ${D}${systemd_system_unitdir}/
	install -m 0644 ${S}/mnt-storage.mount ${D}${systemd_system_unitdir}/
}

FILES_${PN} += "${bindir} \
		${sysconfdir}"

INSANE_SKIP_${PN} = "arch"
