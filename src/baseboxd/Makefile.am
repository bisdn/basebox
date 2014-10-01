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
		-L../roflibs -lroflibs \
                -L../roflibs/netlink -lroflibs_netlink \
                -L../roflibs/ethcore -lroflibs_ethcore \
                -L../roflibs/ipcore -lroflibs_ipcore \
                -L../roflibs/gtpcore -lroflibs_gtpcore \
                -L../roflibs/grecore -lroflibs_grecore


AM_CPPFLAGS = -I$(top_srcdir)/src -I/usr/include/python2.7
AM_LDFLAGS = 

