CRIPTION = "CACert Root and Class3 PKI"
HOMEPAGE = "http://www.cacert.org/index.php?id=3"
SECTION = "misc"
LICENSE = "RDL-COD14"
LIC_FILES_CHKSUM = "file://${COREBASE}/meta/COPYING.MIT;md5=3da9cfbcb788c80a0384361b4de20420"
PR = "r6"

SRC_URI = "file://root.crt file://class3.crt file://DST_Root_CA_X3.pem"

do_install() {
        install -d ${D}${libdir}/ssl/certs
        install -m 0644 ${WORKDIR}/root.crt ${D}${libdir}/ssl/certs/cacert.org.pem
        cat ${WORKDIR}/class3.crt >> ${D}${libdir}/ssl/certs/cacert.org.pem
        install -m 0644 ${WORKDIR}/DST_Root_CA_X3.pem ${D}${libdir}/ssl/certs/

        # Create hash symlinks
        cd ${D}${libdir}/ssl/certs
        ln -s cacert.org.pem e5662767.0
        ln -s cacert.org.pem 5ed36f99.0
        ln -s cacert.org.pem 99d0fa06.0

        ln -s DST_Root_CA_X3.pem 2e5ac55d.0
}

FILES_${PN} = "${libdir}/ssl/certs/*"

