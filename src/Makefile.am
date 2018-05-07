SUBDIRS = of-dpa

sbin_PROGRAMS = baseboxd

PROTO_FILES = \
	grpc/proto/common/empty.proto \
	grpc/proto/common/openconfig-interfaces.proto

GRPC_PROTO_FILES = \
	grpc/proto/statistics/statistics-service.proto

PROTO_SOURCES = \
	common/empty.pb.cc \
	common/empty.pb.h  \
	common/openconfig-interfaces.pb.cc \
	common/openconfig-interfaces.pb.h  \
	statistics/statistics-service.pb.cc \
	statistics/statistics-service.pb.h

GRPC_SOURCES = \
	statistics/statistics-service.grpc.pb.cc \
	statistics/statistics-service.grpc.pb.h

BUILT_SOURCES = \
	$(PROTO_SOURCES) \
	$(GRPC_SOURCES)

CLEANFILES = \
	$(PROTO_SOURCES) \
	$(GRPC_SOURCES)

PROTOS_PATH=$(srcdir)/grpc/proto

$(GRPC_SOURCES): $(GRPC_PROTO_FILES)
	$(PROTOC) -I $(PROTOS_PATH) --grpc_out=. --plugin=protoc-gen-grpc=$(GRPC_CPP_PLUGIN) $^

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
	netlink/netlink-utils.cpp \
	netlink/netlink-utils.hpp \
	netlink/nl_bridge.cpp \
	netlink/nl_bridge.hpp \
	netlink/nl_hashing.hpp \
	netlink/nl_l3.cpp \
	netlink/nl_l3.hpp \
	netlink/nl_obj.cpp \
	netlink/nl_obj.hpp \
	netlink/nl_output.cpp \
	netlink/nl_output.hpp \
	netlink/tap_io.cpp \
	netlink/tap_io.hpp \
	netlink/tap_manager.cpp \
	netlink/tap_manager.hpp

nodist_baseboxd_SOURCES = \
	$(PROTO_SOURCES) \
	$(GRPC_SOURCES)

basebox_grpc_SOURCES = \
	basebox_grpc_statistics.cc \
	basebox_grpc_statistics.h \
	basebox_api.cc \
	basebox_api.h \
	$(PROTO_FILES) \
	$(GRPC_PROTO_FILES)

baseboxd_SOURCES = \
	baseboxd.cpp \
	sai.hpp \
	utils.hpp \
	$(basebox_grpc_SOURCES) \
	$(SOURCES_NETLINK)

baseboxd_LDADD = \
	of-dpa/libbb_ofdpa.la \
	${LIBNL3_LIBS} \
	-lpthread \
	-lrt

AM_CPPFLAGS = -I$(top_srcdir)/src
AM_LDFLAGS =

MAINTAINERCLEANFILES = Makefile.in
