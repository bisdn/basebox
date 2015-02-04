MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

#noinst_LTLIBRARIES = libroflibs_8021core.la
lib_LTLIBRARIES = libroflibs_8021core.la

libroflibs_8021core_la_SOURCES = \
	c8021core.cpp \
	c8021core.hpp 

libroflibs_8021core_la_LIBADD=

library_includedir=$(includedir)/roflibs/8021core
library_include_HEADERS= \
	c8021core.hpp 

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
AM_LDFLAGS=-L../netlink -lroflibs_netlink 
