MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = vmcore

vmcore_SOURCES = main.cc
vmcore_LDADD = -lpthread \
	-lrofl_control \
	-lrofl \
	-lrt \
	-lsoci_core \
	-lsoci_sqlite3

AM_LDFLAGS = -static
