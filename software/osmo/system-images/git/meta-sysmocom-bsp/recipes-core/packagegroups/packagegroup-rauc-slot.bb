DESCRIPTION = "Task for sysmocom rauc slots"
LICENSE = "MIT"
LIC_FILES_CHKSUM = " \
                    file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
ALLOW_EMPTY_${PN} = "1"
PR = "r1"

RDEPENDS_${PN} = "\
	rauc \
	sysmocom-backup-data \
	sysmocom-backup-default \
"

# Add minimal debug helpers
RDEPENDS_${PN} += "\
	strace tcpdump \
"

