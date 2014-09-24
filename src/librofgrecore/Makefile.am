MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = librofgrecore.la

librofgrecore_la_SOURCES = \
	cgrecore.cpp \
	cgrecore.hpp \
	cgreterm.cpp \
	cgreterm.hpp

librofgrecore_la_LIBADD= -lrt 

library_includedir=$(includedir)/rofl/common
library_include_HEADERS= \
	cgrecore.hpp \
	cgreterm.hpp

AM_CPPFLAGS=-fPIC
AM_LDFLAGS=

