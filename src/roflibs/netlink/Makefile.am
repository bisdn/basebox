MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

#noinst_LTLIBRARIES = librofnetlink.la
lib_LTLIBRARIES = librofnetlink.la

librofnetlink_la_SOURCES = \
	cnetlink.hpp \
	cnetlink.cpp \
	cnetdev.hpp \
	cnetdev.cpp \
	ctapdev.hpp \
	ctapdev.cpp \
	ctundev.hpp \
	ctundev.cpp \
	cprefix.hpp \
	cprefix.cpp \
	cpacketpool.hpp \
	cpacketpool.cpp \
	crtroute.hpp \
	crtaddr.hpp \
	crtlink.hpp \
	crtneigh.hpp \
	crtnexthop.hpp \
	clogging.cpp \
	clogging.hpp

librofnetlink_la_LIBADD= -lrt ${LIBNL3_LIBS}

library_includedir=$(includedir)/roflibs/netlink
library_include_HEADERS= \
	cnetlink.hpp \
	cnetdev.hpp \
	ctapdev.hpp \
	ctundev.hpp \
	cprefix.hpp \
	cpacketpool.hpp \
	crtroute.hpp \
	crtaddr.hpp \
	crtlink.hpp \
	crtneigh.hpp \
	crtnexthop.hpp \
	clogging.hpp

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
