MAINTAINERCLEANFILES = Makefile.in

#export INCLUDES += \
#	-I$(abs_srcdir)/roflibs/python \
#	-I$(abs_srcdir)/roflibs/netlink \
#	-I$(abs_srcdir)/roflibs/ipcore \
#	-I$(abs_srcdir)/roflibs/ethcore \
#	-I$(abs_srcdir)/roflibs/gtpcore \
#	-I$(abs_srcdir)/roflibs/grecore

#flowcore 8021core ethcore ipcore gtpcore python
SUBDIRS = netlink of-dpa

#noinst_LTLIBRARIES = libroflibs.la
lib_LTLIBRARIES = libroflibs.la
library_includedir=$(includedir)

libroflibs_la_SOURCES = 
libroflibs_la_LIBADD = \
	netlink/libroflibs_netlink.la
#	flowcore/libroflibs_flowcore.la \
#	ipcore/libroflibs_ipcore.la \
#	8021core/libroflibs_8021core.la \
#	ethcore/libroflibs_ethcore.la \
#	gtpcore/libroflibs_gtpcore.la \
#	python/libroflibs_python.la

#sysconf_DATA = roflibs.conf
