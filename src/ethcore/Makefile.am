MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = ethcored

ethcored_SOURCES = \
	ethcored.cpp \
	cethbase.hpp \
	cethbase.cpp \
	cconfig.hpp \
	cconfig.cpp

	
ethcored_LDADD = -lpthread -lrt -L../librofnetlink -lrofnetlink -L../librofethcore -lrofethcore

AM_CPPFLAGS = -I$(top_srcdir)/src/librofethcore
AM_LDFLAGS = -static
