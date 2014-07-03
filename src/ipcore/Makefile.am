MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = ipcored

ipcored_SOURCES = ipcored.cc \
		cipcore.h \
		cipcore.cc \
		cconfig.h \
		cconfig.cc \
		cdptroute.h \
		cdptroute.cc \
		cdptlink.h \
		cdptlink.cc \
		cdptaddr.h \
		cdptaddr.cc \
		cdptneigh.h \
		cdptneigh.cc \
		cdptnexthop.h \
		cdptnexthop.cc \
		cpacketpool.h \
		cpacketpool.cc \
		cnetdev.h \
		cnetdev.cc \
		ctapdev.h \
		ctapdev.cc \
		cnetlink.h \
		cnetlink.cc \
		flowmod.h
		
ipcored_LDADD = -lpthread -lrt -l../librofcore.la

#AM_CPPFLAGS = -I/usr/local/libnl-3.2.22/include/libnl3
#AM_LDFLAGS = -static -L/usr/local/libnl-3.2.22/lib -lnl-route-3 -lnl-3
