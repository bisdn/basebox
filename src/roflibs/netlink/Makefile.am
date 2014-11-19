MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

#noinst_LTLIBRARIES = libroflibs_netlink.la
lib_LTLIBRARIES = libroflibs_netlink.la

libroflibs_netlink_la_SOURCES = \
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
	cconfig.cpp \
	cconfig.hpp \
	ccookiebox.cpp \
	ccookiebox.hpp

libroflibs_netlink_la_LIBADD= -lrt ${LIBNL3_LIBS}

library_includedir=$(includedir)/roflibs/netlink
library_include_HEADERS= \
	cnetlink.hpp \
	cnetdev.hpp \
	ctapdev.hpp \
	ctundev.hpp \
	cprefix.hpp \
	cpacketpool.hpp \
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
	clogging.hpp \
	cconfig.hpp \
	ccookiebox.hpp

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
