SUBDIRS = of-dpa

sbin_PROGRAMS = baseboxd

PROTO_FILES = \
	grpc/proto/ietf-interfaces.proto \
	grpc/proto/openconfig-interfaces.proto
GRPC_PROTO_FILES = \
	grpc/proto/stats-service.proto

PROTO_SOURCES = \
	ietf-interfaces.pb.cc \
	ietf-interfaces.pb.h \
	openconfig-interfaces.pb.cc \
	openconfig-interfaces.pb.h \
	stats-service.pb.cc \
	stats-service.pb.h

GRPC_SOURCES = \
	stats-service.grpc.pb.cc \
	stats-service.grpc.pb.h

#openconfig-interfaces.proto ietf-interfaces.proto
BUILT_SOURCES = \
	$(PROTO_SOURCES) \
	$(GRPC_SOURCES)

CLEANFILES = \
	$(PROTO_SOURCES) \
	$(GRPC_SOURCES)

PROTOC=/usr/bin/protoc
GRPC_CPP_PLUGIN_PATH=/usr/bin/grpc_cpp_plugin
PROTOS_PATH=$(srcdir)/grpc/proto

$(GRPC_SOURCES): $(GRPC_PROTO_FILES)
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN_PATH) $^

# can be optimized though since protoc is called #PROTO_SOURCES times:
$(PROTO_SOURCES): $(PROTO_FILES) $(GRPC_PROTO_FILES)
	$(PROTOC) -I $(PROTOS_PATH) --cpp_out=. $^

SOURCES_NETLINK = \
	netlink/cnetlink.cpp \
	netlink/cnetlink.hpp \
	netlink/ctapdev.cpp \
	netlink/ctapdev.hpp \
	netlink/nbi_impl.cpp \
	netlink/nbi_impl.hpp \
	netlink/nl_bridge.cpp \
	netlink/nl_bridge.hpp \
	netlink/nl_obj.cpp \
	netlink/nl_obj.hpp \
	netlink/sai.hpp \
	netlink/tap_manager.cpp \
	netlink/tap_manager.hpp

nodist_baseboxd_SOURCES = \
	$(PROTO_SOURCES) \
	$(GRPC_SOURCES)

baseboxd_SOURCES = \
	baseboxd.cpp \
	$(SOURCES_NETLINK)
	
baseboxd_LDADD = \
	of-dpa/libbb_ofdpa.la \
	${LIBNL3_LIBS} \
	-lpthread \
	-lrt

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_LDFLAGS = 

MAINTAINERCLEANFILES = Makefile.in
