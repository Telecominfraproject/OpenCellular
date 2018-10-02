SUMMARY = "Implementation of the JSON-RPC specification for python"
SECTION = "devel/python"
LICENSE = "Apache-2.0"
LIC_FILES_CHKSUM = "file://LICENSE.txt;md5=cdbf8d74b765504fbdf8e154bb4458a1"
PV = "0.1.3+git${SRCPV}"
PR = "r0"

SRCREV = "b59217c971603a30648b041c84f85159afb2ec31"

SRC_URI = "git://github.com/joshmarshall/jsonrpclib.git"
S = "${WORKDIR}/git"

inherit distutils

# python-threading is needed for SimpleJSONRPCServer
RDEPENDS_${PN} = "\
    python-json \
    python-xmlrpc \
    python-threading \
"
