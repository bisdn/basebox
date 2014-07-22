MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = ethcored

ethcored_SOURCES = \
	ethcored.cc \
	ethcore.cc \
	ethcore.h \
	cfib.cc \
	cfib.h \
	cfibentry.h \
	cfibentry.cc \
	sport.h \
	sport.cc \
	logging.h \
	logging.cc \
	cnetlink.cc \
	cnetlink.h \
	cnetdev.cc \
	cnetdev.h \
	ctapdev.cc \
	ctapdev.h \
	cpacketpool.cc \
	cpacketpool.h \
	dptlink.cc \
	dptlink.h \
	cconfig.cc \
	cconfig.h

if AMQP_QMF_SUPPORT
ethcored_SOURCES += qmfagent.h qmfagent.cc
endif


	
ethcored_LDADD = -lpthread -lrt -l ../librofnetlink.la

AM_LDFLAGS = -static
