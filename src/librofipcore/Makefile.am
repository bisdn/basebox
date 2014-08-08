MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = librofipcore.la

librofipcore_la_SOURCES = \
	cipcore.h \
	cipcore.cc \
	cdptaddr.h \
	cdptaddr.cc \
	cdptlink.h \
	cdptlink.cc \
	cdptneigh.h \
	cdptneigh.cc \
	cdptnexthop.h \
	cdptnexthop.cc \
	cdptroute.h \
	cdptroute.cc \
	flowmod.h \
	clinktable.h \
	cnexthoptable.h \
	caddrtable.h \
	cneightable.h \
	croutetable.h \
	croutetables.h

librofipcore_la_LIBADD= -lrt ${LIBNL3_LIBS}

library_includedir=$(includedir)/rofl/common
library_include_HEADERS= \
	cipcore.h \
	cdptaddr.h \
	cdptlink.h \
	cdptneigh.h \
	cdptnexthop.h \
	cdptroute.h \
	flowmod.h \
	clinktable.h \
	cnexthoptable.h \
	caddrtable.h \
	cneightable.h \
	croutetable.h \
	croutetables.h

AM_CPPFLAGS=-fPIC
AM_LDFLAGS=-L../librofnetlink -lrofnetlink
