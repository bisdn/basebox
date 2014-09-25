MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

#noinst_LTLIBRARIES = libroflibs_grecore.la
lib_LTLIBRARIES = libroflibs_grecore.la

libroflibs_grecore_la_SOURCES = \
	cgrecore.cpp \
	cgrecore.hpp \
	cgreterm.cpp \
	cgreterm.hpp

libroflibs_grecore_la_LIBADD= -lrt 

library_includedir=$(includedir)/roflibs/grecore
library_include_HEADERS= \
	cgrecore.hpp \
	cgreterm.hpp

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
AM_LDFLAGS=-L../netlink -lroflibs_netlink
