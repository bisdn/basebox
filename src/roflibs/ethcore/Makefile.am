MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = rstp

#noinst_LTLIBRARIES = libroflibs_ethcore.la
lib_LTLIBRARIES = libroflibs_ethcore.la

libroflibs_ethcore_la_SOURCES = \
	cfibentry.cpp \
	cfibentry.hpp \
	cmemberport.cpp \
	cmemberport.hpp \
	cethendpnt.cpp \
	cethendpnt.hpp \
	cvlan.cpp \
	cvlan.hpp \
	cethcore.cpp \
	cethcore.hpp \
	cethcoredb.hpp \
	cethcoredb.cpp \
	cethcoredb_file.hpp \
	cethcoredb_file.cpp 

libroflibs_ethcore_la_LIBADD=-Lrstp -lroflibs_rstp

library_includedir=$(includedir)/roflibs/ethcore
library_include_HEADERS= \
	cfibentry.hpp \
	cmemberport.hpp \
	cethendpnt.hpp \
	cvlan.hpp \
	cethcore.hpp \
	cethcoredb.hpp \
	cethcoredb_file.hpp

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
AM_LDFLAGS=-L../netlink -lroflibs_netlink 
