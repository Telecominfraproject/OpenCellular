OSMOCONF_FILES = virtual/osmo-bts-virtual.cfg

doc_virtualdir = $(docdir)/examples/osmo-bts-virtual
doc_virtual_DATA = \
        virtual/osmo-bts-virtual.cfg \
        virtual/openbsc-virtual.cfg
EXTRA_DIST = $(doc_virtual_DATA)

if ENABLE_SYSMOBTS
doc_sysmodir = $(docdir)/examples/osmo-bts-sysmo
doc_sysmo_DATA = \
        sysmo/osmo-bts-sysmo.cfg \
        sysmo/sysmobts-mgr.cfg
EXTRA_DIST += $(doc_sysmo_DATA)
OSMOCONF_FILES += sysmo/osmo-bts-sysmo.cfg sysmo/sysmobts-mgr.cfg
endif

if ENABLE_TRX
doc_trxdir = $(docdir)/examples/osmo-bts-trx
doc_trx_DATA = \
        trx/osmo-bts-trx.cfg \
        trx/osmo-bts-trx-calypso.cfg
EXTRA_DIST += $(doc_trx_DATA)
OSMOCONF_FILES += trx/osmo-bts-trx.cfg
endif

if ENABLE_OCTPHY
doc_octphydir = $(docdir)/examples/osmo-bts-octphy
doc_octphy_DATA = \
        octphy/osmo-bts-trx2dsp1.cfg \
        octphy/osmo-bts-octphy.cfg
EXTRA_DIST += $(doc_octphy_DATA)
OSMOCONF_FILES += octphy/osmo-bts-octphy.cfg
endif

if ENABLE_LC15BTS
doc_lc15dir = $(docdir)/examples/osmo-bts-lc15
doc_lc15_DATA = \
        litecell15/osmo-bts-lc15.cfg \
        litecell15/lc15bts-mgr.cfg
EXTRA_DIST += $(doc_lc15_DATA)
OSMOCONF_FILES += litecell15/osmo-bts-lc15.cfg litecell15/lc15bts-mgr.cfg
endif

osmoconfdir = $(sysconfdir)/osmocom
osmoconf_DATA = $(OSMOCONF_FILES)
