MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = libroflibs_netlink.la

libroflibs_netlink_la_SOURCES = \
	cnetlink.cpp \
	cnetlink.hpp \
	cpacketpool.cpp \
	cpacketpool.hpp \
	crtlink.hpp \
	crtlinks.hpp \
	crtneigh.hpp \
	crtneighs.hpp \
	ctapdev.cpp \
	ctapdev.hpp \
	nl_obj.cpp \
	nl_obj.hpp \
	ofdpa_bridge.cpp \
	ofdpa_bridge.hpp \
	sai.hpp \
	tap_manager.cpp \
	tap_manager.hpp

libroflibs_netlink_la_LIBADD= -lrt ${LIBNL3_LIBS}

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
