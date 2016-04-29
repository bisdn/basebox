MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = baseboxd

baseboxd_SOURCES = \
	baseboxd.cpp \
	cbasebox.hpp \
	cbasebox.cpp \
	switch_behavior.hpp \
	switch_behavior.cpp \
	cunixenv.cpp \
	cunixenv.hpp
	
baseboxd_LDADD = -lpthread -lrt \
		-L../roflibs -lroflibs \
		-L../roflibs/netlink -lroflibs_netlink \
		-L../roflibs/of-dpa -lroflibs_ofdpa

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_LDFLAGS = 