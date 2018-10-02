AM_CFLAGS = -Wall -fno-strict-aliasing $(LIBOSMOCORE_CFLAGS) $(LIBOSMOGSM_CFLAGS) $(LIBOSMOVTY_CFLAGS) $(LIBOSMOTRAU_CFLAGS) $(LIBOSMOABIS_CFLAGS) $(LIBOSMOCTRL_CFLAGS) $(LIBOSMOABIS_CFLAGS) $(LIBGPS_CFLAGS) $(ORTP_CFLAGS)
AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include -Iinclude
COMMON_LDADD = $(LIBOSMOCORE_LIBS) $(LIBOSMOGSM_LIBS) $(LIBOSMOVTY_LIBS) $(LIBOSMOTRAU_LIBS) $(LIBOSMOABIS_LIBS) $(LIBOSMOCTRL_LIBS) $(ORTP_LIBS) -ldl

bin_PROGRAMS = osmo-bts-omldummy

osmo_bts_omldummy_SOURCES = main.c bts_model.c
osmo_bts_omldummy_LDADD = $(top_builddir)/src/common/libbts.a $(COMMON_LDADD)
