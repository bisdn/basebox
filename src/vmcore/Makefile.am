MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = vmcore

vmcore_SOURCES = main.cc \
		vmcore.h \
		vmcore.cc \
		dptroute.h \
		dptroute.cc \
		dptlink.h \
		dptlink.cc \
		cpacketpool.h \
		cpacketpool.cc \
		cnetdev.h \
		cnetdev.cc \
		ctapdev.h \
		ctapdev.cc \
		cnetlink.h \
		cnetlink.cc \
		crtlink.h \
		crtlink.cc \
		crtaddr.h \
		crtaddr.cc \
		crtroute.h \
		crtroute.cc \
		crtnexthop.cc \
		crtnexthop.h \
		crtneigh.h \
		crtneigh.cc
		
vmcore_LDADD = -lpthread \
	-lrofl \
	-lrt

AM_CPPFLAGS = -I/home/andreas/local/include/libnl3
AM_LDFLAGS = -static -L/home/andreas/local/lib -lnl-route-3 -lnl-3
