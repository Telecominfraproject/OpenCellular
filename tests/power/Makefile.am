AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include
AM_CFLAGS = -Wall $(LIBOSMOCORE_CFLAGS) $(LIBOSMOCODEC_CFLAGS) $(LIBOSMOGSM_CFLAGS) $(LIBOSMOVTY_CFLAGS) $(LIBOSMOTRAU_CFLAGS) $(ORTP_CFLAGS)
LDADD = $(LIBOSMOCORE_LIBS) $(LIBOSMOCODEC_LIBS) $(LIBOSMOGSM_LIBS) $(LIBOSMOVTY_LIBS) $(LIBOSMOTRAU_LIBS) $(ORTP_LIBS)

noinst_PROGRAMS = power_test
EXTRA_DIST = power_test.ok

power_test_SOURCES = power_test.c $(srcdir)/../stubs.c
power_test_LDADD = $(top_builddir)/src/common/libbts.a $(LIBOSMOABIS_LIBS) $(LDADD)
