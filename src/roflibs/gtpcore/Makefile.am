MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

#noinst_LTLIBRARIES = libroflibs_gtpcore.la
lib_LTLIBRARIES = libroflibs_gtpcore.la

libroflibs_gtpcore_la_SOURCES = \
	cgtpcore.hpp \
	cgtpcore.cpp \
	cgtprelay.hpp \
	cgtprelay.cpp \
	crelay.hpp \
	crelay.cpp \
	cterm.hpp \
	cterm.cpp \
	cport.hpp \
	cteid.hpp \
	clabel.hpp \
	caddress_gtp.hpp \
	ctermdev.hpp \
	ctermdev.cpp \
	cgtpcoredb.hpp \
	cgtpcoredb.cpp \
	cgtpcoredb_file.hpp \
	cgtpcoredb_file.cpp

libroflibs_gtpcore_la_LIBADD= -lrt 

library_includedir=$(includedir)/roflibs/gtpcore
library_include_HEADERS= \
	cgtpcore.hpp \
	cgtprelay.hpp \
	crelay.hpp \
	cterm.hpp \
	cport.hpp \
	cteid.hpp \
	clabel.hpp \
	caddress_gtp.hpp \
	ctermdev.hpp \
	cgtpcoredb.hpp \
	cgtpcoredb_file.hpp

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
AM_LDFLAGS=-L../netlink -lroflibs_netlink 
