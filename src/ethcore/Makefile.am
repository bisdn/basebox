MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = ethcored

ethcored_SOURCES = \
	cvlantable.hpp \
	cvlantable.cpp \
	cvlan.hpp \
	ethcored.cc \
	cethcore.cpp \
	cethcore.hpp \
	cfib.cc \
	cfib.h \
	cfibentry.h \
	cfibentry.cc \
	sport.h \
	sport.cc \
	cdptlink.cc \
	cdptlink.h \
	clinktable.h \
	cconfig.cc \
	cconfig.h

if AMQP_QMF_SUPPORT
ethcored_SOURCES += qmfagent.h qmfagent.cc
endif


	
ethcored_LDADD = -lpthread -lrt -l ../librofnetlink.la

AM_LDFLAGS = -static
