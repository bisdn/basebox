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
		-L../roflibs -lroflibs \
		-L../roflibs/netlink -lroflibs_netlink \
                -L../roflibs/ethcore -lroflibs_ethcore


AM_CPPFLAGS = -I$(top_srcdir)/src
AM_LDFLAGS = 

