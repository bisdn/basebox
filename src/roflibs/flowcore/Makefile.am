MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

#noinst_LTLIBRARIES = libroflibs_flowcore.la
lib_LTLIBRARIES = libroflibs_flowcore.la

libroflibs_flowcore_la_SOURCES = \
	cflowcore.cpp \
	cflowcore.hpp

libroflibs_flowcore_la_LIBADD=

library_includedir=$(includedir)/roflibs/flowcore
library_include_HEADERS= \
	cflowcore.hpp

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
AM_LDFLAGS=-L../netlink -lroflibs_netlink 
