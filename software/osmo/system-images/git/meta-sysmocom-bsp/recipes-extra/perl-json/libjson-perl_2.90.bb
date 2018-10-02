SUMMARY = "The Perl JSON API"
DESCRIPTION = ""
HOMEPAGE = "http://search.cpan.org/~makamaka/JSON-2.90/lib/JSON.pm"
SECTION = "libs"
LICENSE = "Artistic-1.0 | GPL-1.0+"
RDEPENDS_${PN} = " perl-module-carp \
                   perl-module-overload \
                   perl-module-constant \
		   perl-module-base \
		   perl-module-bytes \
"

LIC_FILES_CHKSUM = "file://README;md5=ab4e7db7125faf64419eb9aca79dead6;beginline=1581"

SRC_URI = "http://search.cpan.org/CPAN/authors/id/M/MA/MAKAMAKA/JSON-${PV}.tar.gz"
SRC_URI[md5sum] = "e1512169a623e790a3f69b599cc1d3b9"
SRC_URI[sha256sum] = "4ddbb3cb985a79f69a34e7c26cde1c81120d03487e87366f9a119f90f7bdfe88"

S = "${WORKDIR}/JSON-${PV}"

inherit cpan

BBCLASSEXTEND = "native"
