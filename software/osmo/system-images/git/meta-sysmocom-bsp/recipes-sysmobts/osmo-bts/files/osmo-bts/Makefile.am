AUTOMAKE_OPTIONS = foreign dist-bzip2 1.6

SUBDIRS = include src tests


# package the contrib and doc
EXTRA_DIST = \
	contrib/dump_docs.py contrib/screenrc-l1fwd contrib/osmo-bts-sysmo.service \
	contrib/l1fwd.init contrib/screenrc-sysmobts contrib/respawn.sh \
	doc/examples/sysmo/osmo-bts.cfg \
	doc/examples/sysmo/sysmobts-mgr.cfg \
	doc/examples/virtual/openbsc-virtual.cfg \
	doc/examples/virtual/osmobts-virtual.cfg \
	git-version-gen .version \
	README.md

@RELMAKE@

BUILT_SOURCES = $(top_srcdir)/.version

$(top_srcdir)/.version:
	echo $(VERSION) > $@-t && mv $@-t $@
dist-hook:
	echo $(VERSION) > $(distdir)/.tarball-version
