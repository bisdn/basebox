MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = ethcored

ethcored_SOURCES = \
	ethcored.cc \
	cethbase.hpp \
	cethbase.cpp \
	cethcore.hpp \
	cethcore.cpp \
	cvlantable.hpp \
	cvlantable.cpp \
	cvlan.hpp \
	cfibentry.hpp \
	cfibentry.cpp \
	cdptlink.cc \
	cdptlink.h \
	clinktable.h \
	cconfig.cc \
	cconfig.h

if AMQP_QMF_SUPPORT
ethcored_SOURCES += qmfagent.h qmfagent.cc
endif


	
ethcored_LDADD = -lpthread -lrt -L../librofnetlink -lrofnetlink

AM_LDFLAGS = -static
