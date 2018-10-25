project('baseboxd', 'cpp',
  version: run_command('cat', 'VERSION').stdout().strip(),
  default_options : [
    'cpp_std=c++11',
    'b_asneeded=false',
    'prefix=/usr',
    'sysconfdir=/etc',
  ])

sources = files('''
  src/basebox_api.cc
  src/basebox_api.h
  src/basebox_grpc_statistics.cc
  src/basebox_grpc_statistics.h
  src/baseboxd.cpp
  src/netlink/cnetlink.cpp
  src/netlink/cnetlink.hpp
  src/netlink/ctapdev.cpp
  src/netlink/ctapdev.hpp
  src/netlink/nbi_impl.cpp
  src/netlink/nbi_impl.hpp
  src/netlink/netlink-utils.cpp
  src/netlink/netlink-utils.hpp
  src/netlink/nl_bridge.cpp
  src/netlink/nl_bridge.hpp
  src/netlink/nl_hashing.hpp
  src/netlink/nl_l3.cpp
  src/netlink/nl_l3.hpp
  src/netlink/nl_obj.cpp
  src/netlink/nl_obj.hpp
  src/netlink/nl_output.cpp
  src/netlink/nl_output.hpp
  src/netlink/nl_vlan.cpp
  src/netlink/nl_vlan.hpp
  src/netlink/tap_io.cpp
  src/netlink/tap_io.hpp
  src/netlink/tap_manager.cpp
  src/netlink/tap_manager.hpp
  src/of-dpa/controller.cpp
  src/of-dpa/controller.hpp
  src/of-dpa/ofdpa_datatypes.h
  src/sai.hpp
  src/utils/rofl-utils.hpp
  src/utils/utils.hpp
  '''.split())

# setup paths
prefixdir = get_option('prefix')
systemunitdir = '/usr/lib/systemd/system'
if not prefixdir.startswith('/')
        error('Prefix is not absolute: "@0@"'.format(prefixdir))
endif
libdir = join_paths(prefixdir, get_option('libdir'))
bindir = join_paths(prefixdir, get_option('bindir'))
sbindir = join_paths(prefixdir, get_option('sbindir'))
sysconfdir = join_paths(prefixdir, get_option('sysconfdir'))

# config data
conf_data = configuration_data()
conf_data.set('bindir', bindir)
conf_data.set('sbindir', sbindir)
conf_data.set('libdir', libdir)
conf_data.set('sysconfdir', sysconfdir)
conf_data.set('PACKAGE_VERSION', meson.project_version())

# find programs and libraries
cppc = meson.get_compiler('cpp')

clang_format = find_program('clang-format', required: false, native: true)

protoc = find_program('protoc')
protobuf = dependency('protobuf')

grpc_cpp = find_program('grpc_cpp_plugin')

librofl_common = dependency('rofl_common')
librofl_ofdpa = dependency('rofl_ofdpa')

libgflags = dependency('libgflags', required: false)
if not libgflags.found()
  # find the lib without pkg-config
  libgflags = cppc.find_library('libgflags')
endif

glog = dependency('libglog', version: '>= 0.3.3')

libnl = dependency('libnl-3.0', required: false)
if not libnl.found()
  # find the lib without pkg-config
  libnl = cppc.find_library('libnl-3')
endif

libnl_route = dependency('libnl-route-3.0', required: false)
if not libnl_route.found()
  # find the lib without pkg-config
  libnl_route = cppc.find_library('libnl-route-3')
endif

grpc = dependency('grpc', required: false)
if not grpc.found()
  # find the lib without pkg-config
  grpc = cppc.find_library('grpc')
endif

grpcpp = dependency('grpc++', required: false)
if not grpcpp.found()
  # find the lib without pkg-config
  grpcpp = cppc.find_library('grpc++')
endif

grpc_reflection = cppc.find_library('grpc++_reflection', required: false)

# set up generators
protoc_gen = generator(protoc,
  output    : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
  arguments : ['--proto_path=@CURRENT_SOURCE_DIR@/src/grpc/proto',
    '--cpp_out=@BUILD_DIR@',
    '@INPUT@'])

grpc_gen = generator(protoc,
  output    : ['@BASENAME@.grpc.pb.cc', '@BASENAME@.grpc.pb.h'],
  arguments : ['--proto_path=@CURRENT_SOURCE_DIR@/src/grpc/proto',
    '--grpc_out=@BUILD_DIR@',
    '--plugin=protoc-gen-grpc=' + grpc_cpp.path(),
    '@INPUT@'])

# generate sources and files
src_pb = protoc_gen.process('src/grpc/proto/common/empty.proto',
  'src/grpc/proto/common/openconfig-interfaces.proto',
  'src/grpc/proto/statistics/statistics-service.proto',
  preserve_path_from : meson.current_source_dir()+'/src/grpc/proto')

src_grpc = grpc_gen.process('src/grpc/proto/statistics/statistics-service.proto',
  preserve_path_from : meson.current_source_dir()+'/src/grpc/proto')

version_h = vcs_tag(input: 'src/version.h.in',
  output: 'version.h')

configure_file(input : 'pkg/systemd/baseboxd.service.in',
               output : 'baseboxd.service',
               configuration : conf_data,
               install_dir : systemunitdir)

configure_file(input: 'pkg/release/rpm/baseboxd.spec.in',
               output: 'baseboxd.spec',
	       configuration: conf_data)

# includes
inc = include_directories('src')

# targets
if clang_format.found()
  run_target('clang-format',
    command: [ clang_format, '-i', '-style=file', sources ])
else
  run_target('clang-format',
    command: [ 'echo', 'install', 'clang-format', '&&', 'false' ])
endif

executable('baseboxd',
  sources, src_pb, src_grpc, version_h,
  include_directories: inc,
  dependencies: [
    glog,
    grpc,
    grpc_reflection,
    grpcpp,
    libgflags,
    libnl,
    libnl_route,
    librofl_common,
    librofl_ofdpa,
    protobuf,
  ],
  install: true,
  install_dir: sbindir)
