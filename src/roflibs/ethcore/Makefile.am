MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = rstp

#noinst_LTLIBRARIES = libroflibs_ethcore.la
lib_LTLIBRARIES = libroflibs_ethcore.la

libroflibs_ethcore_la_SOURCES = \
	cfibentry.cpp \
	cfibentry.hpp \
	cmemberport.cpp \
	cmemberport.hpp \
	cvlan.cpp \
	cvlan.hpp \
	cethcore.cpp \
	cethcore.hpp

libroflibs_ethcore_la_LIBADD=-Lrstp -lroflibs_rstp

library_includedir=$(includedir)/roflibs/ethcore
library_include_HEADERS= \
	cfibentry.hpp \
	cmemberport.hpp \
	cvlan.hpp \
	cethcore.hpp

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
AM_LDFLAGS=-L../netlink -lroflibs_netlink 
