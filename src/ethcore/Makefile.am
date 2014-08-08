MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = ethcored

ethcored_SOURCES = \
	ethcored.cc \
	cethbase.hpp \
	cethbase.cpp \
	cconfig.cc \
	cconfig.h

	
ethcored_LDADD = -lpthread -lrt -L../librofnetlink -lrofnetlink -L../librofethcore -lrofethcore

AM_CPPFLAGS = -I$(top_srcdir)/src/librofethcore
AM_LDFLAGS = -static
