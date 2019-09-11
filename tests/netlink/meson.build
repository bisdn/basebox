gtest = dependency('gtest') 

netlink_test_sources = files(''' 
netlink_test.cc
'''.split())

netlink_tests = executable('netlink_tests',
  netlink_test_sources, test_sources, src_pb_test, src_grpc_test, version_h,
  include_directories: inc,
  dependencies: [
    gtest,
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
  ],)

test('netlink test', netlink_tests)
