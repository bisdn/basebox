MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

noinst_LTLIBRARIES = libbb_ofdpa.la

libbb_ofdpa_la_SOURCES = \
	controller.cpp \
	controller.hpp \
	ofdpa_datatypes.h

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src
#AM_LDFLAGS=-L../netlink -lroflibs_netlink
