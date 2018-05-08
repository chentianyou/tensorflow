# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library

# load(
#     "@org_tensorflow//tensorflow/core:platform/default/build_config.bzl",
#     "tf_proto_library",
#     "tf_proto_library_cc",
# )

cc_library(
    name = "hdfs3",
    srcs = glob([
        "src/**/*.cpp",
        "src/**/*.h",
        "build/src/**/*.h",
        "build/src/**/*.cc",
    ]),
    hdrs = glob([
        "BlockLocation.h",
        "DirectoryIterator.h",
        "FileStatus.h",
        "FileSystem.h",
        "FileSystemStats.h",
        "hdfs.h",
        "InputStream.h",
        "OutputStream.h",
        "Permission.h",
        "Exception.h",
        "XmlConfig.h",
    ]),
    includes = [
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
        # ":protos_all_cc",
        "@protobuf_archive//:protobuf",
        "@hornet_dep//:libxml2"
    ],
    linkopts = [
        "-lpthread",
        "-lc++",
    ],
    visibility = ["//visibility:public"],
)

# HDFS_PROTO_SRCS=[
#     "src/proto/ProtobufRpcEngine.proto",
#     "src/proto/datatransfer.proto",
#     "src/proto/RpcHeader.proto",
#     "src/proto/IpcConnectionContext.proto",
#     "src/proto/ClientNamenodeProtocol.proto",
#     "src/proto/ClientDatanodeProtocol.proto",
#     "src/proto/Security.proto",
#     "src/proto/hdfs.proto",
# ]

# tf_proto_library(
#     name = "protos_all",
#     srcs = HDFS_PROTO_SRCS,
#     cc_api_version = 2,
#     default_header = True,
#     go_api_version = 2,
#     j2objc_api_version = 1,
#     java_api_version = 2,
#     js_api_version = 2,
#     visibility = ["//visibility:public"],
# )