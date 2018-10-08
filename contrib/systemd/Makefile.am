if HAVE_SYSTEMD
SYSTEMD_SERVICES = osmo-bts-virtual.service

if ENABLE_SYSMOBTS
SYSTEMD_SERVICES += osmo-bts-sysmo.service sysmobts-mgr.service
endif

if ENABLE_TRX
SYSTEMD_SERVICES += osmo-bts-trx.service
endif

if ENABLE_LC15BTS
SYSTEMD_SERVICES += osmo-bts-lc15.service lc15bts-mgr.service
endif

EXTRA_DIST = $(SYSTEMD_SERVICES)
systemdsystemunit_DATA = $(SYSTEMD_SERVICES)
endif # HAVE_SYSTEMD
