MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = vmcored

vmcored_SOURCES = vmcored.cc \
		vmcore.h \
		vmcore.cc \
		dptroute.h \
		dptroute.cc \
		dptlink.h \
		dptlink.cc \
		dptaddr.h \
		dptaddr.cc \
		dptneigh.h \
		dptneigh.cc \
		dptnexthop.h \
		dptnexthop.cc \
		cpacketpool.h \
		cpacketpool.cc \
		cnetdev.h \
		cnetdev.cc \
		ctapdev.h \
		ctapdev.cc \
		cnetlink.h \
		cnetlink.cc \
		crtlink.h \
		crtlink.cc \
		crtaddr.h \
		crtaddr.cc \
		crtroute.h \
		crtroute.cc \
		crtnexthop.cc \
		crtnexthop.h \
		crtneigh.h \
		crtneigh.cc \
		flowmod.h
		
vmcored_LDADD = -lpthread \
	-lrt

#AM_CPPFLAGS = -I/usr/local/libnl-3.2.22/include/libnl3
#AM_LDFLAGS = -static -L/usr/local/libnl-3.2.22/lib -lnl-route-3 -lnl-3
