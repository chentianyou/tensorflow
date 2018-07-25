# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library

load(
    "@org_tensorflow//tensorflow/core:platform/default/build_config.bzl",
    "tf_proto_library",
    "tf_proto_library_cc",
)

HORNET_COPTS=[
    # "-std=c++11",
    # "-stdlib=libc++",
]

cc_library(
    name = "dbcommon",
    srcs = glob([
        "dbcommon/src/dbcommon/**/*.h",
        "dbcommon/src/dbcommon/**/*.cc",
        "dbcommon/build/codegen/src/**/*.cc",
        "dbcommon/build/codegen/src/**/*.h",],
        exclude=[
            "dbcommon/src/dbcommon/function/typecast-func.cc",
            "dbcommon/src/dbcommon/function/func.cc",
            "dbcommon/src/dbcommon/function/typecast-func.h",
            "dbcommon/src/dbcommon/function/func.h",
            "dbcommon/src/dbcommon/type/type-util.cc",
        ],
    ),
    hdrs = glob([
        "dbcommon/src/dbcommon/**/*.h",
    ]),
    includes = [
        "hdfs",
        "thrift",
        "dbcommon/src",
        "dbcommon/build/codegen/src",
    ],
    deps = [
        "@hornet_dep//:thrift",
        "@hornet_dep//:glog",
        "@hornet_dep//:iconv",
        "@hdfs3//:hdfs3",
        "@zlib_archive//:zlib",
    ],
    copts = HORNET_COPTS,
    linkopts = [
        "-lpthread",
        "-lc++",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "univplan",
    srcs = glob([
        "univplan/src/univplan/**/*.h",
        "univplan/src/univplan/**/*.cc",
    ]),
    includes = [
        "univplan/src",
        # "univplan/build/src",
    ],
    deps = [
        ":protos_plan_cc_impl",
        ":dbcommon",
        "@protobuf_archive//:protobuf",
    ],
    alwayslink=1,
    copts = HORNET_COPTS,
    visibility = ["//visibility:public"],
)

tf_proto_library(
    name = "protos_plan",
    srcs = [
        "univplan/src/univplan/proto/universal-plan.proto",
        "univplan/src/univplan/proto/universal-plan-expr.proto",
        "univplan/src/univplan/proto/universal-plan-catalog.proto",
    ],
    cc_api_version = 2,
    default_header = True,
    visibility = ["//visibility:public"],
)

# cc_library(
#     name = "rpc",
#     srcs = glob([
#         "rpc/build/src/rpc/**/*.h",
#         "rpc/build/src/rpc/**/*.cc",
#         "rpc/src/rpc/**/*.h",
#         "rpc/src/rpc/**/*.cc",
#     ]),
#     hdrs = glob([
#         "rpc/src/rpc/**/*.h",
#     ]),
#     deps = [
#         ":dbcommon",
#         # ":protos_rpc_cc",
#         "@protobuf_archive//:protobuf",
#     ],
#     includes =[
#         "rpc/src",
#         "rpc/build/src",
#     ],
#     copts = HORNET_COPTS,
#     linkopts = [
#         "-lpthread",
#         "-lc++",
#     ],
#     visibility = ["//visibility:public"],
# )

# tf_proto_library(
#     name = "protos_rpc",
#     srcs = [
#         "rpc/src/rpc/proto/myrpc.proto"
#     ],
#     cc_api_version = 2,
#     default_header = True,
#     go_api_version = 2,
#     j2objc_api_version = 1,
#     java_api_version = 2,
#     js_api_version = 2,
#     visibility = ["//visibility:public"],
# )

#magma-clien
# cc_library(
#     name = "kv-client",
#     srcs = glob([
#         "kv/src/kv/client/**/*.h",
#         "kv/src/kv/client/**/*.cc",
#         "kv/src/kv/cwrapper/**/*.h",
#         "kv/src/kv/cwrapper/**/*.cc",
#     ]),
#     hdrs = glob([
#         "kv/src/kv/client/**/*.h",
#         "kv/src/kv/cwrapper/**/*.h",
#     ]),
#     deps = [
#         ":rpc",
#         ":univplan",
#         ":dbcommon",
#         # ":protos_magma_cc",
#         "@hornet_dep//:glog",
#         # "@grpc//:grpc++",
#         "@protobuf_archive//:protobuf",
#     ],
#     includes = [
#         "kv/src"
#     ],
#     copts = HORNET_COPTS,
#     linkopts = [
#         "-lpthread",
#         "-lc++",
#     ],
#     visibility = ["//visibility:public"],
# )

# MAGMA_PROTO_SRC = [
#     "kv/src/kv/client/common/batchrpc/protos/batchrpc.proto",
#     "kv/src/kv/client/common/coord/protos/kvservice.proto",
# ]

# tf_proto_library(
#     name = "protos_magma",
#     srcs = MAGMA_PROTO_SRC,
#     cc_api_version = 2,
#     default_header = True,
#     go_api_version = 2,
#     j2objc_api_version = 1,
#     java_api_version = 2,
#     js_api_version = 2,
#     visibility = ["//visibility:public"],
# )

cc_library(
    name = "storage",
    srcs = glob([
        # "storage/build/src/storage/**/*.h",
        # "storage/build/src/storage/**/*.cc",
        "storage/src/storage/**/*.h",
        "storage/src/storage/**/*.cc",
    ]),
    hdrs = glob([
        "storage/src/storage/**/*.h",
    ]),
    deps = [
        ":univplan",
        ":dbcommon",
        ":orc_proto_cc_impl",
        "@hdfs3//:hdfs3",
        "@hornet_dep//:lz4",
        "@hornet_dep//:glog",
        "@hornet_dep//:iconv",
        "@snappy",
        "@zlib_archive//:zlib",
        "@protobuf_archive//:protobuf",
        "@jsoncpp_git//:jsoncpp",
    ],
    includes = [
        "storage/src",
        "storage/build/src",
    ],
    copts = HORNET_COPTS,
    linkopts = [
        "-lpthread",
        "-lc++",
    ],
    visibility = ["//visibility:public"],
)

ORC_PROTO_SRCS = [
    "storage/src/storage/format/orc/orc_proto.proto"
]

tf_proto_library(
    name = "orc_proto",
    srcs = ORC_PROTO_SRCS,
    cc_api_version = 2,
    default_header = True,
    visibility = ["//visibility:public"],
)
