MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = librofethcore.la

librofethcore_la_SOURCES = \
	cdpid.hpp \
	cfibentry.cpp \
	cfibentry.hpp \
	cmemberport.cpp \
	cmemberport.hpp \
	cvlan.cpp \
	cvlan.hpp \
	cvlantable.cpp \
	cvlantable.hpp

librofethcore_la_LIBADD= -lrt ${LIBNL3_LIBS}

library_includedir=$(includedir)/rofl/common
library_include_HEADERS= \
	cdpid.hpp \
	cfibentry.hpp \
	cmemberport.hpp \
	cvlan.hpp \
	cvlantable.hpp

AM_CPPFLAGS=-fPIC
