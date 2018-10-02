AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include
AM_CFLAGS = -Wall -fno-strict-aliasing $(LIBOSMOCORE_CFLAGS) $(LIBOSMOGSM_CFLAGS) $(LIBOSMOCODEC_CFLAGS) $(LIBOSMOCODING_CFLAGS) $(LIBOSMOVTY_CFLAGS) $(LIBOSMOTRAU_CFLAGS) $(LIBOSMOABIS_CFLAGS) $(LIBOSMOCTRL_CFLAGS) $(ORTP_CFLAGS)
LDADD = $(LIBOSMOCORE_LIBS) $(LIBOSMOGSM_LIBS) $(LIBOSMOCODEC_LIBS) $(LIBOSMOCODING_LIBS) $(LIBOSMOVTY_LIBS) $(LIBOSMOTRAU_LIBS) $(LIBOSMOABIS_LIBS) $(LIBOSMOCTRL_LIBS) $(ORTP_LIBS) -ldl

EXTRA_DIST = trx_if.h l1_if.h loops.h

bin_PROGRAMS = osmo-bts-trx

osmo_bts_trx_SOURCES = main.c trx_if.c l1_if.c scheduler_trx.c trx_vty.c loops.c
osmo_bts_trx_LDADD = $(top_builddir)/src/common/libl1sched.a $(top_builddir)/src/common/libbts.a $(LDADD)

