MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = vmcore

vmcore_SOURCES = main.cc \
		vmcore.h \
		vmcore.cc \
		cnetdev.h \
		cnetdev.cc \
		cpacketpool.h \
		cpacketpool.cc \
		ctapdev.h \
		ctapdev.cc \
		cnetlink.h \
		cnetlink.cc \
		crtlink.h \
		crtlink.cc
vmcore_LDADD = -lpthread \
	-lrofl \
	-lrt \
	-lsoci_core \
	-lsoci_sqlite3

AM_CPPFLAGS = -I/home/andreas/local/include/libnl3
AM_LDFLAGS = -static -L/home/andreas/local/lib -lnl-route-3 -lnl-3
