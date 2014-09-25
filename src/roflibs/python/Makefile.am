MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

#noinst_LTLIBRARIES = libroflibs_python.la
lib_LTLIBRARIES = libroflibs_python.la

libroflibs_python_la_SOURCES = \
	cpython.cpp \
	cpython.hpp

libroflibs_python_la_LIBADD= -lrt 

library_includedir=$(includedir)/roflibs/python
library_include_HEADERS= \
	cpython.hpp

AM_CPPFLAGS=-fPIC 
AM_CXXFLAGS=${PYTHON_INCLUDES} -I/usr/include/python2.7
AM_LDFLAGS=-L/usr/lib64 -lpython2.7

