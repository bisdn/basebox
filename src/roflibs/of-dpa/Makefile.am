MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

lib_LTLIBRARIES = libroflibs_ofdpa.la

libroflibs_ofdpa_la_SOURCES = \
	switch_behavior_ofdpa.cpp \
	switch_behavior_ofdpa.hpp

libroflibs_ofdpa_la_LIBADD = 

library_includedir=$(includedir)/roflibs/ofdpa
library_include_HEADERS = 

AM_CPPFLAGS=-fPIC -std=c++11
AM_CXXFLAGS=-I$(top_srcdir)/src
AM_LDFLAGS=-L../netlink -lroflibs_netlink
