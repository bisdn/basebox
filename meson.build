project('baseboxd', 'cpp',
  version: run_command('cat', 'VERSION').stdout().strip(),
  default_options : [
    'cpp_std=c++17',
    'b_asneeded=false',
    'prefix=/usr',
    'sysconfdir=/etc',
  ])

sources = files('''
  src/basebox_api.cc
  src/basebox_api.h
  src/basebox_grpc_statistics.cc
  src/basebox_grpc_statistics.h
  src/baseboxd.cc
  src/netlink/cnetlink.cc
  src/netlink/cnetlink.h
  src/netlink/ctapdev.cc
  src/netlink/ctapdev.h
  src/netlink/nbi_impl.cc
  src/netlink/nbi_impl.h
  src/netlink/netlink-utils.cc
  src/netlink/netlink-utils.h
  src/netlink/nl_bond.cc
  src/netlink/nl_bond.h
  src/netlink/nl_bridge.cc
  src/netlink/nl_bridge.h
  src/netlink/nl_hashing.h
  src/netlink/nl_interface.cc
  src/netlink/nl_interface.h
  src/netlink/nl_l3.cc
  src/netlink/nl_l3.h
  src/netlink/nl_l3_interfaces.h
  src/netlink/nl_obj.cc
  src/netlink/nl_obj.h
  src/netlink/nl_output.cc
  src/netlink/nl_output.h
  src/netlink/nl_route_query.h
  src/netlink/nl_vlan.cc
  src/netlink/nl_vlan.h
  src/netlink/nl_vxlan.cc
  src/netlink/nl_vxlan.h
  src/netlink/tap_io.cc
  src/netlink/tap_io.h
  src/netlink/tap_manager.cc
  src/netlink/tap_manager.h
  src/of-dpa/controller.cc
  src/of-dpa/controller.h
  src/of-dpa/ofdpa_client.cc
  src/of-dpa/ofdpa_client.h
  src/of-dpa/ofdpa_datatypes.h
  src/sai.h
  src/utils/rofl-utils.h
  src/utils/utils.h
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
clang_tidy_script = find_program('run-clang-tidy.py', required: false)
clang_tidy = find_program('clang-tidy', required: false)

protoc = find_program('protoc')
protobuf = dependency('protobuf')

grpc_cpp = find_program('grpc_cpp_plugin')

librofl_common = dependency('rofl_common')
librofl_ofdpa = dependency('rofl_ofdpa', version: '>= 1.1.0')

libgflags = dependency('libgflags', required: false)
if not libgflags.found()
  # find the lib without pkg-config
  libgflags = cppc.find_library('libgflags')
endif

libsystemd = dependency('libsystemd', required: false)
if not libsystemd.found()
  # find the lib without pkg-config
  libsystemd = cppc.find_library('libsystemd')
endif

glog = dependency('libglog', version: '>= 0.3.3', required: false)
if not glog.found()
  # find the lib without pkg-config
  glog = cppc.find_library('libglog')
endif

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
src_pb = protoc_gen.process(
  'src/grpc/proto/api/ofdpa.proto',
  'src/grpc/proto/common/empty.proto',
  'src/grpc/proto/common/openconfig-interfaces.proto',
  'src/grpc/proto/statistics/statistics-service.proto',
  preserve_path_from : meson.current_source_dir()+'/src/grpc/proto')

src_grpc = grpc_gen.process(
  'src/grpc/proto/api/ofdpa.proto',
  'src/grpc/proto/statistics/statistics-service.proto',
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

if meson.version() < '0.50'
  # targets
  if clang_format.found()
    run_target('clang-format',
      command: [ clang_format, '-i', '-style=file', sources ])
  else
    run_target('clang-format',
      command: [ 'echo', 'install', 'clang-format', '&&', 'false' ])
  endif
endif

if clang_tidy.found()
  if clang_tidy_script.found()
    # prefer script with parallel execution
    run_target(
      'clang-tidy',
      command: [ 'scripts/run-clang-tidy-script.sh' ] )
  else
    run_target(
      'clang-tidy',
      command: [
          clang_tidy, '-fix', '-p', meson.build_root(),
      ] + sources)
  endif
else
  run_target('clang-tidy',
    command: [ 'echo', 'install', 'clang-tidy', '&&', 'false' ])
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
    libsystemd,
    protobuf,
  ],
  install: true,
  install_dir: bindir)
