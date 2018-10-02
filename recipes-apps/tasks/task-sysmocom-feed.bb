DESCRIPTION = "Package to force building everything we want to provide"
LICENSE = "MIT"
LIC_FILES_CHKSUM = " \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
ALLOW_EMPTY_${PN} = "1"
PR = "r22"

RDEPENDS_${PN} = "\
	task-sysmocom-tools \
	task-sysmocom-debug \
        task-sysmocom-legacy \
	minicom vlan patch procps psmisc \
	ppp rsync sed usbutils openvpn iperf \
	lcr cronie iproute2 i2c-tools cu \
	python-pyserial python-pexpect bridge-utils \
	pciutils nfacct logrotate dnsmasq ifupdown \
	logrotate python-jsonrpclib python-enum iputils \
	packagegroup-sysmobts-sob rtl8169-eeprom autossh \
	perl libdbd-sqlite-perl libdbi-perl libjson-perl \
	netcat-openbsd perf lksctp-tools task-gprscore \
	osmo-sip-connector \
	"
