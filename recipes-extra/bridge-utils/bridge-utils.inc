SUMMARY = "Tools for ethernet bridging"
HOMEPAGE = "http://www.linuxfoundation.org/collaborate/workgroups/networking/bridge"
SECTION = "console/network"
LICENSE = "GPLv2"

DEPENDS = "sysfsutils"

SRC_URI = "${SOURCEFORGE_MIRROR}/bridge/bridge-utils-${PV}.tar.gz \
          file://bridge-utils-1.5-check-error-returns-from-write-to-sysfs.patch \
          file://bridge-utils-1.5-fix-error-message-for-incorrect-command.patch \
          file://bridge-utils-1.5-fix-incorrect-command-in-manual.patch \
	  file://ifupdown.sh file://bridge-utils.sh \
"

inherit autotools update-alternatives
B = "${S}"

ALTERNATIVE_${PN} = "brctl"
ALTERNATIVE_PRIORITY[brctl] = "100"
ALTERNATIVE_LINK_NAME[brctl] = "${sbindir}/brctl"

EXTRA_OECONF = "--with-linux-headers=${STAGING_INCDIR}"

do_install_append () {
    install -d ${D}/${datadir}/bridge-utils
    install -d ${D}/${sysconfdir}/network/if-pre-up.d
    install -d ${D}/${sysconfdir}/network/if-post-down.d

    # copy the scripts (from debian bridge-utils patch)
    install -d ${D}/lib/bridge-utils
    install -m 0755 ${WORKDIR}/ifupdown.sh ${D}/lib/bridge-utils/ifupdown.sh
    install -m 0755 ${WORKDIR}/bridge-utils.sh ${D}/lib/bridge-utils/bridge-utils.sh

    # create symlinks for ifupdown magic
    cd ${D}${sysconfdir}/network/if-pre-up.d
    ln -s /lib/bridge-utils/ifupdown.sh bridge
    cd ${D}${sysconfdir}/network/if-post-down.d
    ln -s /lib/bridge-utils/ifupdown.sh bridge
}

FILES_${PN} += "/lib/bridge-utils/*.sh"

RRECOMMENDS_${PN} = "kernel-module-bridge"
