MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = librofcore.la

librofcore_la_SOURCES = \
	cnetlink.h \
	cnetlink.cc \
	cnetdev.h \
	cnetdev.cc \
	ctapdev.h \
	ctapdev.cc \
	cpacketpool.h \
	cpacketpool.cc \
	crtroute.h \
	crtroute.cc \
	crtaddr.cc \
	crtaddr.h \
	crtlink.cc \
	crtlink.h \
	crtneigh.cc \
	crtneigh.h \
	crtnexthop.cc \
	crtnexthop.h \
	logging.cc \
	logging.h

librofcore_la_LIBADD= -lrt ${LIBNL3_LIBS}

library_includedir=$(includedir)/rofl/common
library_include_HEADERS= \
	cnetlink.h \
	cnetdev.h \
	ctapdev.h \
	cpacketpool.h \
	crtroute.h \
	crtaddr.h \
	crtlink.h \
	crtneigh.h \
	crtnexthop.h \
	logging.h

AM_CPPFLAGS=-fPIC
