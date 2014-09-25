MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = librofgtpcore.la

librofgtpcore_la_SOURCES = \
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
	ctermdev.cpp

librofgtpcore_la_LIBADD= -lrt 

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
	ctermdev.hpp

AM_CPPFLAGS=-fPIC
AM_LDFLAGS=-L../librofnetlink -lrofnetlink

