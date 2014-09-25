MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = librofpython.la

librofpython_la_SOURCES = \
	cpython.cpp \
	cpython.hpp

librofpython_la_LIBADD= -lrt 

library_includedir=$(includedir)/roflibs/python
library_include_HEADERS= \
	cpython.hpp

AM_CPPFLAGS=-fPIC 
AM_CXXFLAGS=${PYTHON_INCLUDES} -I/usr/include/python2.7
AM_LDFLAGS=-L/usr/lib64 -lpython2.7

