MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = librofrstp

noinst_LTLIBRARIES = librofethcore.la

librofethcore_la_SOURCES = \
	cfibentry.cpp \
	cfibentry.hpp \
	cmemberport.cpp \
	cmemberport.hpp \
	cvlan.cpp \
	cvlan.hpp \
	cethcore.cpp \
	cethcore.hpp

librofethcore_la_LIBADD= -lrt ${LIBNL3_LIBS}

library_includedir=$(includedir)/roflibs/ethcore
library_include_HEADERS= \
	cfibentry.hpp \
	cmemberport.hpp \
	cvlan.hpp \
	cethcore.hpp

AM_CPPFLAGS=-fPIC
AM_LDFLAGS=-L../librofnetlink -lrofnetlink -Llibrofrstp -lrofrstp

