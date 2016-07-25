MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = baseboxd

baseboxd_SOURCES = \
	baseboxd.cpp
	
baseboxd_LDADD = -lpthread \
		 -lrt \
		 ../roflibs/libroflibs.la

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_LDFLAGS = 
