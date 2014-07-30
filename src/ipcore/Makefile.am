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
		flowmod.h \
		clinktable.h \
		caddrtable.h \
		cneightable.h \
		cnexthoptable.h \
		croutetable.h \
		croutetables.h
		
ipcored_LDADD = -lpthread -lrt -L../librofnetlink -lrofnetlink

#AM_CPPFLAGS = -I/usr/local/libnl-3.2.22/include/libnl3
#AM_LDFLAGS = -static -L/usr/local/libnl-3.2.22/lib -lnl-route-3 -lnl-3
