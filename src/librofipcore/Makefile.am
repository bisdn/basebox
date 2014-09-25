MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = librofipcore.la

librofipcore_la_SOURCES = \
	cipcore.hpp \
	cipcore.cpp \
	clink.hpp \
	clink.cpp \
	caddr.hpp \
	caddr.cpp \
	cneigh.hpp \
	cneigh.cpp \
	croutetable.hpp \
	croute.hpp \
	croute.cpp \
	cnexthop.hpp \
	cnexthop.cpp

librofipcore_la_LIBADD= -lrt ${LIBNL3_LIBS}

library_includedir=$(includedir)/roflibs/ipcore
library_include_HEADERS= \
	cipcore.hpp \
	clink.hpp \
	caddr.hpp \
	cneigh.hpp \
	croutetable.hpp \
	croute.hpp \
	cnexthop.hpp

AM_CPPFLAGS=-fPIC -std=c++11
AM_LDFLAGS=-L../librofnetlink -lrofnetlink
