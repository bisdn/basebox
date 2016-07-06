MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = libroflibs_netlink.la

libroflibs_netlink_la_SOURCES = \
	cnetlink.hpp \
	cnetlink.cpp \
	cnetlink_observer.hpp \
	tap_manager.hpp \
	tap_manager.cpp \
	ctapdev.hpp \
	ctapdev.cpp \
	cpacketpool.hpp \
	cpacketpool.cpp \
	crtlink.hpp \
	crtlinks.hpp \
	crtneigh.hpp \
	crtneighs.hpp \
	clogging.cpp \
	clogging.hpp \
	nl_obj.hpp \
	nl_obj.cpp

libroflibs_netlink_la_LIBADD= -lrt ${LIBNL3_LIBS}

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
