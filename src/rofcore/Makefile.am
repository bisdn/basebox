MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = rofcored

rofcored_SOURCES = rofcored.cpp \
		croflog.hpp \
		croflog.cpp \
		crofconf.hpp \
		crofconf.cpp \
		crofproxies.hpp \
		crofproxies.cpp \
		crofproxy.hpp \
		crofproxy.cpp
		
rofcored_LDADD = -lpthread \
	-lrt

#AM_CPPFLAGS = -I/usr/local/libnl-3.2.22/include/libnl3
#AM_LDFLAGS = -static -L/usr/local/libnl-3.2.22/lib -lnl-route-3 -lnl-3
