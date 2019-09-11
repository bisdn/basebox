protoc_gen_test = generator(protoc,
  output    : ['@BASENAME@.pb.cc', '@BASENAME@.pb.h'],
  arguments : ['--proto_path=' + project_source_root + '/src/grpc/proto',
    '--cpp_out=@BUILD_DIR@',
    '@INPUT@'])

grpc_gen_test = generator(protoc,
  output    : ['@BASENAME@.grpc.pb.cc', '@BASENAME@.grpc.pb.h'],
  arguments : ['--proto_path=' + project_source_root + '/src/grpc/proto',
    '--grpc_out=@BUILD_DIR@',
    '--plugin=protoc-gen-grpc=' + grpc_cpp.path(),
    '@INPUT@'])

# generate sources and files
src_pb_test = protoc_gen_test.process(
  project_source_root+'/src/grpc/proto/api/ofdpa.proto',
  project_source_root+'/src/grpc/proto/common/empty.proto',
  project_source_root+'/src/grpc/proto/common/openconfig-interfaces.proto',
  project_source_root+'/src/grpc/proto/statistics/statistics-service.proto',
  preserve_path_from : project_source_root+'/src/grpc/proto')

src_grpc_test = grpc_gen_test.process(
  project_source_root+'/src/grpc/proto/api/ofdpa.proto',
  project_source_root+'/src/grpc/proto/statistics/statistics-service.proto',
  preserve_path_from :  project_source_root+'/src/grpc/proto')

subdir('netlink')
