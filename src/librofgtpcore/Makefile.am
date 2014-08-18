MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = librofgtpcore.la

librofgtpcore_la_SOURCES = \
	cgtpcore.hpp \
	cgtpcore.cpp \
	cbearer.hpp \
	cbearer.cpp \
	cterm.hpp \
	cterm.cpp

librofgtpcore_la_LIBADD= -lrt 

library_includedir=$(includedir)/rofl/common
library_include_HEADERS= \
	cgtpcore.hpp \
	cbearer.hpp \
	cterm.hpp

AM_CPPFLAGS=-fPIC
AM_LDFLAGS=
