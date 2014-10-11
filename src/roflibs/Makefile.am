MAINTAINERCLEANFILES = Makefile.in

#export INCLUDES += \
#	-I$(abs_srcdir)/roflibs/python \
#	-I$(abs_srcdir)/roflibs/netlink \
#	-I$(abs_srcdir)/roflibs/ipcore \
#	-I$(abs_srcdir)/roflibs/ethcore \
#	-I$(abs_srcdir)/roflibs/gtpcore \
#	-I$(abs_srcdir)/roflibs/grecore

SUBDIRS = netlink ipcore ethcore gtpcore grecore python

#noinst_LTLIBRARIES = libroflibs.la
lib_LTLIBRARIES = libroflibs.la
library_includedir=$(includedir)

libroflibs_la_SOURCES = 
libroflibs_la_LIBADD = \
	netlink/libroflibs_netlink.la \
	ipcore/libroflibs_ipcore.la \
	ethcore/libroflibs_ethcore.la \
	gtpcore/libroflibs_gtpcore.la \
	grecore/libroflibs_grecore.la \
	python/libroflibs_python.la

#sysconf_DATA = roflibs.conf
