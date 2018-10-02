
SUMMARY = "Debian bootstrap"
DESCRIPTION = "This install debootstrap to bootstrap debian distributions"
HOMEPAGE = "http://packages.debian.org/source/sid/debootstrap"
SECTION = "extra"
LICENSE="MIT"
LIC_FILES_CHKSUM = "file://debian/copyright;md5=1e68ced6e1689d4cd9dac75ff5225608"

SRC_URI = "${DEBIAN_MIRROR}/main/d/debootstrap/debootstrap_${PV}.tar.gz"
PR = "r6"
RDEPENDS_${PN} = "wget gnupg coreutils"

inherit autotools

# Skip compiling as we will require fakeroot
fakeroot do_compile() {
	base_do_compile
}

do_install() {
	autotools_do_install
}
