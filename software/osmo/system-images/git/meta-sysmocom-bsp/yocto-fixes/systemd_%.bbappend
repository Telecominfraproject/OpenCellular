require recipes-fixes/${PN}/${PN}_sysmocom.inc

EXTRA_OECONF_append = " \
                 --disable-smack --disable-libcurl --disable-backlight --disable-vconsole \
                 --disable-hibernate --disable-kdbus --disable-seccomp --disable-gcrypt \
                 --disable-importd --disable-coredump --disable-hwdb --disable-libidn \
                 --disable-libiptc --disable-bootchart --disable-logind --disable-apparmor \
                 --disable-selinux --disable-quotacheck --disable-polkit \
"
