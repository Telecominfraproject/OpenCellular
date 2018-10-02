# PN/PV are wrong inside the .inc file with 1.19.1
SYSMOCOM_ORIG_PV := "${PV}"
require recipes-core/busybox/${PN}_sysmocom.inc
require recipes-core/busybox/${PN}_sysmocom_systemd.inc
