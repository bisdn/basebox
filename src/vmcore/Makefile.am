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
		clinkcache.h \
		clinkcache.cc
vmcore_LDADD = -lpthread \
	-lrofl \
	-lrt \
	-lsoci_core \
	-lsoci_sqlite3

AM_CPPFLAGS = -I/usr/include/libnl3
AM_LDFLAGS = -static -lnl-route-3 -lnl-3
