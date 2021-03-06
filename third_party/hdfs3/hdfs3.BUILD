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
        "build/src/platform.h",]),
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
        "src/proto-tf",
        "src/common",
        "build/src",
        "src",
    ],
    deps = [
        "@boringssl//:ssl",
        ":protos_all_cc_impl",
        "@hornet_dep//:libxml2",
        "@hornet_dep//:gsasl",
        "@curl//:curl"
    ],
    copts = [
        "-Dprivate=public",
        "-Dprotected=public",
    ],
    linkopts = [
        "-lpthread",
        "-lc++",
        "-luuid",
        "-lkrb5",
    ],
    visibility = ["//visibility:public"],
)

HDFS_PROTO_SRCS=[
    "src/proto-tf/ProtobufRpcEngine.proto",
    "src/proto-tf/datatransfer.proto",
    "src/proto-tf/RpcHeader.proto",
    "src/proto-tf/IpcConnectionContext.proto",
    "src/proto-tf/ClientNamenodeProtocol.proto",
    "src/proto-tf/ClientDatanodeProtocol.proto",
    "src/proto-tf/Security.proto",
    "src/proto-tf/hdfs.proto",
    "src/proto-tf/encryption.proto",
]

tf_proto_library(
    name = "protos_all",
    srcs = HDFS_PROTO_SRCS,
    cc_api_version = 2,
    default_header = True,
    visibility = ["//visibility:public"],
)
