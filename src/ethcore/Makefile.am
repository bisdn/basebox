MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = ethcored

ethcored_SOURCES = \
	ethcored.cc \
	cethbase.hpp \
	cethbase.cpp \
	cdptlink.cc \
	cdptlink.h \
	clinktable.h \
	cconfig.cc \
	cconfig.h

if AMQP_QMF_SUPPORT
ethcored_SOURCES += qmfagent.h qmfagent.cc
endif


	
ethcored_LDADD = -lpthread -lrt -L../librofnetlink -lrofnetlink -L../librofethcore -lrofethcore

AM_CPPFLAGS = -I../librofethcore
AM_LDFLAGS = -static
