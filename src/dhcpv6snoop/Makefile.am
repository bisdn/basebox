MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = dhcpv6snoop

dhcpv6snoop_SOURCES = main.cc \
		dhcpv6snoop.h \
		dhcpv6snoop.cc \
		cdhcp_option.h \
		cdhcp_option.cc \
		cdhcp_option_ia_pd.h \
		cdhcp_option_ia_pd.cc
		
dhcpv6snoop_LDADD = -lpthread \
	-lrofl \
	-lrt

AM_CPPFLAGS = 
AM_LDFLAGS = 
