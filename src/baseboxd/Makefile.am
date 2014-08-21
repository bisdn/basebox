MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = baseboxd

baseboxd_SOURCES = \
	baseboxd.cpp \
	cbasebox.hpp \
	cbasebox.cpp \
	cconfig.hpp \
	cconfig.cpp

	
baseboxd_LDADD = -lpthread -lrt \
		-L../librofnetlink -lrofnetlink \
		-L../librofethcore -lrofethcore \
		-L../librofipcore -lrofipcore \
		-L../librofgtpcore -lrofgtpcore

AM_CPPFLAGS = -I$(top_srcdir)/src/librofethcore -I$(top_srcdir)/src/librofipcore -I$(top_srcdir)/src/librofgtpcore
AM_LDFLAGS = -static

