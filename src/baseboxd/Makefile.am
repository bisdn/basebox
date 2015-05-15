MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = baseboxd

baseboxd_SOURCES = \
	baseboxd.cpp \
	cbasebox.hpp \
	cbasebox.cpp \
	cconfig.hpp \
	cconfig.cpp \
	switch_behavior.hpp \
	switch_behavior.cpp

	
baseboxd_LDADD = -lpthread -lrt \
		-L../roflibs -lroflibs \
		-L../roflibs/netlink -lroflibs_netlink \
		-L../roflibs/of-dpa -lroflibs_ofdpa
#                -L../roflibs/flowcore -lroflibs_flowcore \
#                -L../roflibs/ethcore -lroflibs_ethcore \
#                -L../roflibs/ipcore -lroflibs_ipcore \
#                -L../roflibs/gtpcore -lroflibs_gtpcore \
#                -L../roflibs/python -lroflibs_python


AM_CPPFLAGS = -I$(top_srcdir)/src -I/usr/include/python2.7
AM_LDFLAGS = 

