MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = ethcore

ethcore_SOURCES = \
	main.cc \
	ethcore.cc \
	ethcore.h \
	cfib.cc \
	cfib.h \
	cfibentry.h \
	cfibentry.cc \
	sport.h \
	sport.cc
ethcore_LDADD = -lpthread -lrt 

AM_LDFLAGS = -static
