MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = rofcored

rofcored_SOURCES = \
	rofcored.cpp \
	crofbase.hpp \
	crofbase.cpp \
	cconfig.hpp \
	cconfig.cpp

	
rofcored_LDADD = -lpthread -lrt -L../librofnetlink -lrofnetlink -L../librofethcore -lrofethcore

AM_CPPFLAGS = -I$(top_srcdir)/src/librofethcore
AM_LDFLAGS = -static
