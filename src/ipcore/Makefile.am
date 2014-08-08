MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = ipcored

ipcored_SOURCES = ipcored.cc \
		cconfig.h \
		cconfig.cc
		
ipcored_LDADD = -lpthread -lrt -L../librofnetlink -lrofnetlink -L../librofipcore -lrofipcore

AM_CPPFLAGS = -I$(top_srcdir)/src/librofipcore
#AM_CPPFLAGS = -I/usr/local/libnl-3.2.22/include/libnl3
#AM_LDFLAGS = -static -L/usr/local/libnl-3.2.22/lib -lnl-route-3 -lnl-3
