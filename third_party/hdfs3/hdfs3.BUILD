# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library

load(
    "@org_tensorflow//tensorflow/core:platform/default/build_config.bzl",
    "tf_proto_library",
    "tf_proto_library_cc",
)

cc_library(
    name = "hdfs3",
    srcs = glob([
        "src/**/*.cpp",
        "src/**/*.h",
        # "build/src/**/*.h",
        # "build/src/**/*.cc",
        "build/src/platform.h",
    ]),
    hdrs = glob([
        "src/client/BlockLocation.h",
        "src/client/DirectoryIterator.h",
        "src/client/FileStatus.h",
        "src/client/FileSystem.h",
        "src/client/FileSystemStats.h",
        "src/client/hdfs.h",
        "src/client/InputStream.h",
        "src/client/OutputStream.h",
        "src/client/Permission.h",
        "src/common/Exception.h",
        "src/common/XmlConfig.h",
    ]),
    includes = [
        "src/client",
        "src/common",
        "src/proto",
        "build/src",
        "src",
    ],
    copts = [
        "-Dprivate=public",
        "-Dprotected=public",
    ],
    deps = [
        ":protos_all_cc_impl",
        "@protobuf_archive//:protobuf",
        "@hornet_dep//:libxml2",
        "@hornet_dep//:gsasl",
    ],
    linkopts = [
        "-lkrb5",
    ],
    visibility = ["//visibility:public"],
)

HDFS_PROTO_SRCS=[
    "src/proto/ProtobufRpcEngine.proto",
    "src/proto/datatransfer.proto",
    "src/proto/RpcHeader.proto",
    "src/proto/IpcConnectionContext.proto",
    "src/proto/ClientNamenodeProtocol.proto",
    "src/proto/ClientDatanodeProtocol.proto",
    "src/proto/Security.proto",
    "src/proto/hdfs.proto",
]

tf_proto_library_cc(
    name = "protos_all",
    srcs = HDFS_PROTO_SRCS,
    include = "src/proto",
    default_header = True,
    # cc_grpc_version = 1,
    visibility = ["//visibility:public"],
)