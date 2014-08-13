MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = librofipcore.la

librofipcore_la_SOURCES = \
	cipcore.hpp \
	cipcore.cpp \
	caddr.hpp \
	caddr.cpp \
	clink.hpp \
	clink.cpp \
	cneigh.hpp \
	cneigh.cpp \
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
	cipcore.hpp \
	caddr.hpp \
	clink.hpp \
	cneigh.hpp \
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
