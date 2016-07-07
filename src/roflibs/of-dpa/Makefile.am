MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = libroflibs_ofdpa.la

libroflibs_ofdpa_la_SOURCES = \
	cbasebox.cpp \
	cbasebox.hpp \
	ofdpa_datatypes.hpp

libroflibs_ofdpa_la_LIBADD = 

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
AM_LDFLAGS=-L../netlink -lroflibs_netlink
