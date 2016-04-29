MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = netlink of-dpa

noinst_LTLIBRARIES = libroflibs.la
library_includedir=$(includedir)

libroflibs_la_SOURCES = 
libroflibs_la_LIBADD = \
	netlink/libroflibs_netlink.la \
	of-dpa/libroflibs_ofdpa.la
