MAINTAINERCLEANFILES = Makefile.in

SUBDIRS =

noinst_LTLIBRARIES = libbb_ofdpa.la

libbb_ofdpa_la_SOURCES = \
	controller.cpp \
	controller.hpp \
	ofdpa_client.cpp \
	ofdpa_client.hpp \
	ofdpa_datatypes.h \
	$(GRPC_PROTO_FILES)

nodist_libbb_ofdpa_la_SOURCES = \
	$(PROTO_SOURCES) \
	$(GRPC_SOURCES)

AM_CPPFLAGS=-fPIC
AM_CXXFLAGS=-I$(top_srcdir)/src

GRPC_PROTO_FILES = \
	grpc/ofdpa.proto

PROTO_SOURCES = \
	ofdpa.pb.cc \
	ofdpa.pb.h

GRPC_SOURCES = \
	ofdpa.grpc.pb.cc \
	ofdpa.grpc.pb.h

BUILT_SOURCES = \
	$(PROTO_SOURCES) \
	$(GRPC_SOURCES)

CLEANFILES = \
	$(PROTO_SOURCES) \
	$(GRPC_SOURCES)


PROTOS_PATH=$(srcdir)/grpc

$(GRPC_SOURCES): $(GRPC_PROTO_FILES)
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) $^

# can be optimized though since protoc is called #PROTO_SOURCES times:
$(PROTO_SOURCES): $(GRPC_PROTO_FILES)
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $^

