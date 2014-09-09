MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = ethboxd

ethboxd_SOURCES = \
	ethboxd.cpp \
	cethbox.hpp \
	cethbox.cpp \
	cconfig.hpp \
	cconfig.cpp

	
ethboxd_LDADD = -lpthread -lrt \
		-L../librofnetlink -lrofnetlink \
		-L../librofethcore -lrofethcore 

AM_CPPFLAGS = -I$(top_srcdir)/src/librofethcore 
AM_LDFLAGS = -static

