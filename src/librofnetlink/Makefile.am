MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = librofnetlink.la

librofnetlink_la_SOURCES = \
	cnetlink.h \
	cnetlink.cc \
	cnetdev.h \
	cnetdev.cc \
	ctapdev.h \
	ctapdev.cc \
	ctundev.h \
	ctundev.cc \
	cprefix.hpp \
	cprefix.cpp \
	cpacketpool.h \
	cpacketpool.cc \
	crtroute.h \
	crtaddr.h \
	crtlink.h \
	crtneigh.h \
	crtnexthop.h \
	clogging.cc \
	clogging.h

librofnetlink_la_LIBADD= -lrt ${LIBNL3_LIBS}

library_includedir=$(includedir)/rofl/common
library_include_HEADERS= \
	cnetlink.h \
	cnetdev.h \
	ctapdev.h \
	ctundev.h \
	cprefix.hpp \
	cpacketpool.h \
	crtroute.h \
	crtaddr.h \
	crtlink.h \
	crtneigh.h \
	crtnexthop.h \
	clogging.h

AM_CPPFLAGS=-fPIC
