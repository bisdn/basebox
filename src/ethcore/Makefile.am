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
	qmfagent.h \
	qmfagent.cc
ethcored_LDADD = -lpthread -lrt 

AM_LDFLAGS = -static
