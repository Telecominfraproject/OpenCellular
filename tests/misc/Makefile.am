AM_CPPFLAGS = $(all_includes) -I$(top_srcdir)/include
AM_CFLAGS = -Wall $(LIBOSMOCORE_CFLAGS) $(LIBOSMOGSM_CFLAGS) $(LIBOSMOCODEC_CFLAGS) \
	$(LIBOSMOABIS_CFLAGS) $(LIBOSMOTRAU_CFLAGS)
LDADD = $(LIBOSMOCORE_LIBS) $(LIBOSMOGSM_LIBS) $(LIBOSMOCODEC_LIBS) \
	$(LIBOSMOABIS_LIBS) $(LIBOSMOTRAU_LIBS)
noinst_PROGRAMS = misc_test
EXTRA_DIST = misc_test.ok

misc_test_SOURCES = misc_test.c $(srcdir)/../stubs.c
misc_test_LDADD = $(top_builddir)/src/common/libbts.a \
		$(LDADD)
