PACKAGES += "${PN}-misc"
ALLOW_EMPTY_${PN}-misc = "1"

RDEPENDS_${PN}-misc = "${PN}-ss ${PN}-nstat ${PN}-ifstat ${PN}-rtacct ${PN}-lnstat"
