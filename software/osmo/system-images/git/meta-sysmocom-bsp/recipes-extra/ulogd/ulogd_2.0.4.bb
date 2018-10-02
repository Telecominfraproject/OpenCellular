DESCRIPTION = "Userspace logging daemon for netfilter/iptables related logging"
HOMEPAGE = "http://www.netfilter.org/projects/ulogd/index.html"
LICENSE = "GPLv2"
LIC_FILES_CHKSUM = "file://COPYING;md5=c93c0550bd3173f4504b2cbd8991e50b"
DEPENDS = "libnfnetlink libnetfilter-log libnetfilter-conntrack libmnl libnetfilter-acct sqlite3 jansson"
PR = "r4"

SRC_URI = " \
	http://www.netfilter.org/projects/ulogd/files/ulogd-${PV}.tar.bz2;name=tar \
	file://init \
	file://0001-configure.ac-Add-without-mysql-pgsql.patch \
	"
SRC_URI[tar.md5sum] = "7c71ec460dfea5287eba27472c521ebc"
SRC_URI[tar.sha256sum] = "56b30a13a8832e97178f39b7bb173a0b1dfe173dbb60d99a1a386c0962a2effd"

PARALLEL_MAKE = ""

inherit autotools update-rc.d

INITSCRIPT_NAME = "ulogd"

EXTRA_OECONF = "--without-mysql --without-pgsql --without-dbi"

do_install_append() {
	install -d ${D}/${sysconfdir}/init.d
	install -m 0755 ${WORKDIR}/init ${D}/${sysconfdir}/init.d/ulogd
}

PACKAGES =+ "ulogd-in-nfacct ulogd-in-nfct ulogd-in-nflog ulogd-in-ulog ulogd-out-sqlite3 ulogd-out-json ulogd-out-pcap"

FILES_ulogd-in-nfacct = "${libdir}/ulogd/ulogd_inpflow_NFACCT.so"
RDEPENDS_ulogd-in-nfacct += "kernel-module-nfnetlink-acct kernel-module-xt-nfacct"

FILES_ulogd-in-nfct = "${libdir}/ulogd/ulogd_inpflow_NFCT.so"
RDEPENDS_ulogd-in-nfct += "kernel-module-nf-conntrack-netlink"

FILES_ulogd-in-nflog = "${libdir}/ulogd/ulogd_inppkt_NFLOG.so"
RDEPENDS_ulogd-in-nflog += "kernel-module-nfnetlink-log kernel-module-xt-nflog"

FILES_ulogd-in-ulog = "${libdir}/ulogd/ulogd_inppkt_ULOG.so"

FILES_ulogd-out-sqlite3 = "${libdir}/ulogd/ulogd_output_SQLITE3.so"
FILES_ulogd-out-json = "${libdir}/ulogd/ulogd_output_JSON.so"
FILES_ulogd-out-pcap = "${libdir}/ulogd/ulogd_output_PCAP.so"
FILES_ulogd-out-json = "${libdir}/ulogd/ulogd_output_JSON.so"
