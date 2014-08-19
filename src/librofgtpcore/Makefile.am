MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = librofgtpcore.la

librofgtpcore_la_SOURCES = \
	cgtpcore.hpp \
	cgtpcore.cpp \
	crelay.hpp \
	cterm.hpp \
	cterm.cpp \
	cport.hpp \
	cteid.hpp \
	clabel.hpp \
	caddress_gtp.hpp

librofgtpcore_la_LIBADD= -lrt 

library_includedir=$(includedir)/rofl/common
library_include_HEADERS= \
	cgtpcore.hpp \
	crelay.hpp \
	cterm.hpp \
	cport.hpp \
	cteid.hpp \
	clabel.hpp \
	caddress_gtp.hpp

AM_CPPFLAGS=-fPIC
AM_LDFLAGS=
