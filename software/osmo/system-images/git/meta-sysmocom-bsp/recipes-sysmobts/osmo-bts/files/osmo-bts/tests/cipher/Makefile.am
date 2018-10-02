AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include
AM_CFLAGS = -Wall $(LIBOSMOCORE_CFLAGS) $(LIBOSMOGSM_CFLAGS) $(LIBOSMOVTY_CFLAGS) $(LIBOSMOTRAU_CFLAGS) $(LIBOSMOCODEC_CFLAGS) $(ORTP_CFLAGS)
LDADD = $(LIBOSMOCORE_LIBS) $(LIBOSMOGSM_LIBS) $(LIBOSMOVTY_LIBS) $(LIBOSMOTRAU_LIBS) $(LIBOSMOABIS_LIBS) $(LIBOSMOCODEC_LIBS) $(ORTP_LIBS)
noinst_PROGRAMS = cipher_test
EXTRA_DIST = cipher_test.ok

cipher_test_SOURCES = cipher_test.c $(srcdir)/../stubs.c
cipher_test_LDADD = $(top_builddir)/src/common/libbts.a $(LDADD)
