AM_CFLAGS = -Wall -fno-strict-aliasing $(LIBOSMOCORE_CFLAGS) $(LIBOSMOGSM_CFLAGS) $(LIBOSMOVTY_CFLAGS) $(LIBOSMOTRAU_CFLAGS) $(LIBOSMOABIS_CFLAGS) $(LIBOSMOCTRL_CFLAGS) $(LIBOSMOABIS_CFLAGS) $(LIBGPS_CFLAGS) $(ORTP_CFLAGS)
AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include -Iinclude
COMMON_LDADD = $(LIBOSMOCORE_LIBS) $(LIBOSMOGSM_LIBS) $(LIBOSMOVTY_LIBS) $(LIBOSMOTRAU_LIBS) $(LIBOSMOABIS_LIBS) $(LIBOSMOCTRL_LIBS) $(ORTP_LIBS) -ldl

noinst_HEADERS = l1_if.h osmo_mcast_sock.h virtual_um.h

bin_PROGRAMS = osmo-bts-virtual

osmo_bts_virtual_SOURCES = main.c bts_model.c virtualbts_vty.c scheduler_virtbts.c l1_if.c virtual_um.c osmo_mcast_sock.c
osmo_bts_virtual_LDADD = $(top_builddir)/src/common/libl1sched.a $(top_builddir)/src/common/libbts.a $(COMMON_LDADD)
