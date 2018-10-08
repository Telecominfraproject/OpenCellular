AUTOMAKE_OPTIONS = foreign dist-bzip2 1.6

SUBDIRS = include src tests doc contrib


# package the contrib and doc
EXTRA_DIST = \
	contrib/dump_docs.py contrib/screenrc-l1fwd \
	contrib/l1fwd.init contrib/screenrc-sysmobts contrib/respawn.sh \
	git-version-gen .version \
	README.md

DISTCHECK_CONFIGURE_FLAGS = \
	--with-systemdsystemunitdir=$$dc_install_base/$(systemdsystemunitdir)

@RELMAKE@

BUILT_SOURCES = $(top_srcdir)/.version

$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version
