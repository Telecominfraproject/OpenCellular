AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include
AM_CFLAGS = -Wall $(LIBOSMOCORE_CFLAGS) $(LIBOSMOGSM_CFLAGS) $(LIBOSMOCODEC_CFLAGS) $(LIBOSMOTRAU_CFLAGS) $(LIBOSMOABIS_CFLAGS)
LDADD = $(LIBOSMOCORE_LIBS) $(LIBOSMOGSM_LIBS) $(LIBOSMOCODEC_LIBS) $(LIBOSMOTRAU_LIBS) $(LIBOSMOABIS_LIBS)
noinst_PROGRAMS = tx_power_test
EXTRA_DIST = tx_power_test.ok tx_power_test.err

tx_power_test_SOURCES = tx_power_test.c $(srcdir)/../stubs.c
tx_power_test_LDADD = $(top_builddir)/src/common/libbts.a $(LDADD)
