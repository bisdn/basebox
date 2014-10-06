MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

#noinst_LTLIBRARIES = libroflibs_ipcore.la
lib_LTLIBRARIES = libroflibs_ipcore.la

libroflibs_ipcore_la_SOURCES = \
	cipcore.hpp \
	cipcore.cpp \
	clink.hpp \
	clink.cpp \
	caddr.hpp \
	caddr.cpp \
	cneigh.hpp \
	cneigh.cpp \
	croutetable.hpp \
	croute.hpp \
	croute.cpp \
	cnexthop.hpp \
	cnexthop.cpp

libroflibs_ipcore_la_LIBADD= 
#libroflibs_ipcore_la_LIBADD= -lrt ${LIBNL3_LIBS}

library_includedir=$(includedir)/roflibs/ipcore
library_include_HEADERS= \
	cipcore.hpp \
	clink.hpp \
	caddr.hpp \
	cneigh.hpp \
	croutetable.hpp \
	croute.hpp \
	cnexthop.hpp

AM_CPPFLAGS=-fPIC -std=c++11
AM_CXXFLAGS=-I$(top_srcdir)/src
AM_LDFLAGS=-L../netlink -lroflibs_netlink

sysconf_DATA = roflibs.conf
