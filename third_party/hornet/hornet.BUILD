# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library

load(
    "@org_tensorflow//tensorflow/core:platform/default/build_config.bzl",
    "tf_proto_library",
    "tf_proto_library_cc",
)

load(
    "@org_tensorflow//third_party:hornet/hornet.bzl", 
    "cogapp_gen",
    "cogapp_gen_prepare",
    "thrift_gen")

cogapp_gen_prepare()

cogapp_gen(
    name = "cogapp_gen_h",
    srcs = [
        "dbcommon/src/dbcommon/function/arith-cmp-func.h",
        "dbcommon/src/dbcommon/function/typecast-func.h",
        "dbcommon/src/dbcommon/function/func-kind.h",
    ],
)

cogapp_gen(
    name = "cogapp_gen_cc",
    srcs = [
        "dbcommon/src/dbcommon/function/arith-func.cc",
        "dbcommon/src/dbcommon/function/cmp-func.cc",
        "dbcommon/src/dbcommon/function/typecast-func.cc",
        "dbcommon/src/dbcommon/function/func.cc",
        "dbcommon/src/dbcommon/type/type-util.cc",
    ]
)

thrift_gen(
    name = "hive_thrift"
)

cc_library(
    name = "dbcommon",
    srcs = glob([
        "dbcommon/src/dbcommon/**/*.cc",
        ],
        exclude=[
            "dbcommon/src/dbcommon/function/typecast-func.cc",
            "dbcommon/src/dbcommon/function/func.cc",
            "dbcommon/src/dbcommon/function/typecast-func.h",
            "dbcommon/src/dbcommon/function/func.h",
            "dbcommon/src/dbcommon/type/type-util.cc",
        ],
    ) + [
        ":cogapp_gen_h", 
        ":cogapp_gen_cc",
        ":hive_thrift"],
    hdrs = glob([
        "dbcommon/src/dbcommon/**/*.h",
        "dbcommon/build/codegen/src/**/*.h",
    ]),
    includes = [
        "hdfs",
        "thrift",
        "dbcommon/src",
        "dbcommon/src/dbcommon/filesystem/hive"
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

