FIRMWARE-VERSION = "${PV}"
PR = "r2.${INC_PR}"

SRC_URI[md5sum] = "c7b75b5ebc5bb1185afd2880444b10dd"
SRC_URI[sha256sum] = "c1e9817a4f7163396ee00577877bcb61bab9fbb252947ea05a43ea872d53d515"

require ${PN}.inc

do_install() {
	install -d ${D}/usr/bin

	install -m 0755 ${S}/sbts2050-util ${D}/usr/bin/sbts2050-util
}

FILES_${PN} = "/usr/bin/sbts2050-util"
