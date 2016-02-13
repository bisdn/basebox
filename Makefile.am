MAINTAINERCLEANFILES = \
	$(top_srcdir)/m4/libtool.m4 \
	$(top_srcdir)/m4/lt~obsolete.m4 \
	$(top_srcdir)/m4/ltoptions.m4 \
	$(top_srcdir)/m4/ltsugar.m4 \
	$(top_srcdir)/m4/ltversion.m4 \
	$(top_srcdir)/configure \
	$(top_srcdir)/aclocal.m4 \
	$(top_srcdir)/Makefile.in

SUBDIRS = src

EXTRA_DIST = \
	autogen.sh \
	README.md \
	VERSION

install-exec-hook:
	@echo "Install"
uninstall-hook:
	@echo "Uninstall"

maintainer-clean-local:
	rm -rf $(top_srcdir)/build-aux

ACLOCAL_AMFLAGS=-I m4
