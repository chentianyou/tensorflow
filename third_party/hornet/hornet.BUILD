# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library

load(
    "@org_tensorflow//tensorflow/core:platform/default/build_config.bzl",
    "tf_proto_library",
    "tf_proto_library_cc",
)

cc_library(
    name = "dbcommon",
    srcs = glob([
        "dbcommon/src/dbcommon/**/*.cc",
        "dbcommon/build/codegen/src/**/*.cc",
        "dbcommon/build/codegen/src/**/*.cpp",
        "dbcommon/build/codegen/src/**/*.h",
        ],
        exclude=[
            "dbcommon/src/dbcommon/function/typecast-func.cc",
            "dbcommon/src/dbcommon/function/func.cc",
            "dbcommon/src/dbcommon/function/typecast-func.h",
            "dbcommon/src/dbcommon/function/func.h",
            "dbcommon/src/dbcommon/type/type-util.cc",
            "dbcommon/build/codegen/src/ThriftHiveMetastore_server.skeleton.cpp",
            "dbcommon/build/codegen/src/FacebookService_server.skeleton.cpp",
        ],
    ),
    hdrs = glob([
        "dbcommon/src/dbcommon/**/*.h",
        "dbcommon/build/codegen/src/**/*.h",
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
        "@snappy//:snappy",
    ],
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
    ],
    deps = [
        ":protos_plan_cc_impl",
        ":dbcommon",
    ],
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

cc_library(
    name = "storage",
    srcs = glob([
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
        "@lz4//:lz4",
        "@hornet_dep//:glog",
        "@hornet_dep//:iconv",
        "@snappy//:snappy",
        "@zlib_archive//:zlib",
        "@jsoncpp_git//:jsoncpp",
    ],
    includes = [
        "storage/src",
        "storage/build/src",
    ],
    # alwayslink=1,
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
