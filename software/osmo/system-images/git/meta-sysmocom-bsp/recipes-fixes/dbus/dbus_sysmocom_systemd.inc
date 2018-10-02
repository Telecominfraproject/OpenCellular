PRINC = "1"

# We do not want to install a dbus-daemon by default. systemd is using
# dbus without the dbus-daemon being installed. Patch out the dbus daemon.
RRECOMMENDS_${PN}-lib = ""
