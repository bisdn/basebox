MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = libroflibs_netlink.la

libroflibs_netlink_la_SOURCES = \
	cnetlink.hpp \
	cnetlink.cpp \
	cnetlink_observer.hpp \
	cnetdev.hpp \
	cnetdev.cpp \
	ctapdev.hpp \
	ctapdev.cpp \
	ctundev.hpp \
	ctundev.cpp \
	cpacketpool.hpp \
	cpacketpool.cpp \
	crtroute.hpp \
	crtroutes.hpp \
	crtaddr.hpp \
	crtaddrs.hpp \
	crtlink.hpp \
	crtlinks.hpp \
	crtneigh.hpp \
	crtneighs.hpp \
	crtnexthop.hpp \
	crtnexthops.hpp \
	clogging.cpp \
	clogging.hpp \
	ccookiebox.cpp \
	ccookiebox.hpp

libroflibs_netlink_la_LIBADD= -lrt ${LIBNL3_LIBS}

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
