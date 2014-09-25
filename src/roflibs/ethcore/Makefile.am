MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = rstp

#noinst_LTLIBRARIES = librofethcore.la
lib_LTLIBRARIES = librofethcore.la

librofethcore_la_SOURCES = \
	cfibentry.cpp \
	cfibentry.hpp \
	cmemberport.cpp \
	cmemberport.hpp \
	cvlan.cpp \
	cvlan.hpp \
	cethcore.cpp \
	cethcore.hpp

librofethcore_la_LIBADD= -lrt ${LIBNL3_LIBS} -Lrstp -lrofrstp

library_includedir=$(includedir)/roflibs/ethcore
library_include_HEADERS= \
	cfibentry.hpp \
	cmemberport.hpp \
	cvlan.hpp \
	cethcore.hpp

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
AM_LDFLAGS=-L../netlink -lrofnetlink -Lrstp -lrofrstp
