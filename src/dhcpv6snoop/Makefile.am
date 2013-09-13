MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = dhcpv6snoop

dhcpv6snoop_SOURCES = main.cc \
		cdhcpv6snoop.h \
		cdhcpv6snoop.cc \
		cdhcp_option.h \
		cdhcp_option.cc \
		cdhcp_option_ia_pd.h \
		cdhcp_option_ia_pd.cc \
		cdhcp_option_ia_pd_option_prefix.h \
		cdhcp_option_ia_pd_option_prefix.cc \
		cdhcpmsg.h \
		cdhcpmsg.cc \
		cdhcpmsg_clisrv.h \
		cdhcpmsg_clisrv.cc \
		cdhcpmsg_relay.h \
		cdhcpmsg_relay.cc \
		cppcap.h \
		cppcap.cc
		
dhcpv6snoop_LDADD = -lpthread \
	-lrofl \
	-lrt \
	-lpcap

AM_CPPFLAGS = 
AM_LDFLAGS = 
