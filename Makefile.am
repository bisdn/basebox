MAINTAINERCLEANFILES = \
	$(top_srcdir)/Makefile.in \
	$(top_srcdir)/aclocal.m4 \
	$(top_srcdir)/configure \
	$(top_srcdir)/m4/libtool.m4 \
	$(top_srcdir)/m4/ltoptions.m4 \
	$(top_srcdir)/m4/ltsugar.m4 \
	$(top_srcdir)/m4/ltversion.m4 \
	$(top_srcdir)/m4/lt~obsolete.m4

SUBDIRS = src

EXTRA_DIST = \
	README.md \
	VERSION \
	autogen.sh \
	pkg/systemd/baseboxd.service \
	pkg/systemd/sysconfig.template

install-exec-hook:
	@echo "Install"
uninstall-hook:
	@echo "Uninstall"

maintainer-clean-local:
	rm -rf $(top_srcdir)/build-aux

ACLOCAL_AMFLAGS=-I m4
