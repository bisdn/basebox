MAINTAINERCLEANFILES = Makefile.in

SUBDIRS = 

sbin_PROGRAMS = ethcored

ethcored_SOURCES = \
	ethcored.cc \
	cethbase.hpp \
	cethbase.cpp \
	cvlantable.hpp \
	cvlantable.cpp \
	cvlan.hpp \
	cvlan.cpp \
	cfibentry.hpp \
	cfibentry.cpp \
	cmemberport.hpp \
	cmemberport.cpp \
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
