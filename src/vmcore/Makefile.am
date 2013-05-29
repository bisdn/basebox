MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = vmcore

vmcore_SOURCES = main.cc \
		cnetlink.cc \
		croute.cc \
		vmcore.cc
vmcore_LDADD = -lpthread \
	-lrofl_control \
	-lrofl \
	-lrt \
	-lsoci_core \
	-lsoci_sqlite3

AM_CPPFLAGS = -I/home/andi/local/include/libnl3
AM_LDFLAGS = -static /home/andi/local/lib/libnl-route-3.a /home/andi/local/lib/libnl-3.a
