# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library

# load(
#     "@org_tensorflow//tensorflow/core:platform/default/build_config.bzl",
#     "tf_proto_library",
#     "tf_proto_library_cc",
# )

HORNET_COPTS=[
    "-g",
    "-fno-limit-debug-info",
    "-fno-omit-frame-pointer",
    "-fno-strict-aliasing",
    "-mavx",
    "-mno-avx2",
    "-DAVX_OPT",
    "-std=c++11",
    "-Wno-deprecated-register",
    "-stdlib=libc++",
    "-O3",
]

cc_library(
    name = "dbcommon",
    srcs = glob([
        "dbcommon/src/dbcommon/log/**/*.h",
        "dbcommon/src/dbcommon/log/**/*.cc",
        "dbcommon/src/dbcommon/nodes/**/*.h",
        "dbcommon/src/dbcommon/nodes/**/*.cc",
        "dbcommon/src/dbcommon/utils/**/*.h",
        "dbcommon/src/dbcommon/utils/**/*.cc",
        "dbcommon/src/dbcommon/common/**/*.h",
        "dbcommon/src/dbcommon/common/**/*.cc",
        "dbcommon/src/dbcommon/function/array-function.h",
        "dbcommon/src/dbcommon/function/invoker.h",
        "dbcommon/src/dbcommon/function/binary-function.cc",
        "dbcommon/src/dbcommon/function/string-binary-function.h",
        "dbcommon/src/dbcommon/function/agg-func.cc",
        "dbcommon/src/dbcommon/function/cmp-func.cc",
        "dbcommon/src/dbcommon/function/string-function.cc",
        "dbcommon/src/dbcommon/function/agg-func.h",
        "dbcommon/src/dbcommon/function/func-kind.h",
        # "dbcommon/src/dbcommon/function/typecast-func.cc",
        "dbcommon/src/dbcommon/function/arith-cmp-func.h",
        # "dbcommon/src/dbcommon/function/func.cc",
        # "dbcommon/src/dbcommon/function/typecast-func.h",
        "dbcommon/src/dbcommon/function/arith-func.cc",
        # "dbcommon/src/dbcommon/function/func.h",
        "dbcommon/src/dbcommon/function/volatile-func.cc",
        "dbcommon/src/dbcommon/function/array-function.cc",
        "dbcommon/src/dbcommon/function/invoker.cc",
        "dbcommon/src/dbcommon/function/volatile-func.h",
        "dbcommon/src/dbcommon/filesystem/**/*.h",
        "dbcommon/src/dbcommon/filesystem/**/*.cc",
        "dbcommon/src/dbcommon/thread/**/*.h",
        "dbcommon/src/dbcommon/thread/**/*.cc",
        "dbcommon/src/dbcommon/checksum/**/*.h",
        "dbcommon/src/dbcommon/checksum/**/*.cc",
        "dbcommon/src/dbcommon/type/bool.h",
        "dbcommon/src/dbcommon/type/integer.cc",
        "dbcommon/src/dbcommon/type/type-util.h",
        "dbcommon/src/dbcommon/type/date.cc",
        "dbcommon/src/dbcommon/type/integer.h",
        "dbcommon/src/dbcommon/type/typebase.cc",
        "dbcommon/src/dbcommon/type/array.cc",
        "dbcommon/src/dbcommon/type/date.h",
        "dbcommon/src/dbcommon/type/type-kind.h",
        "dbcommon/src/dbcommon/type/typebase.h",
        "dbcommon/src/dbcommon/type/array.h",
        "dbcommon/src/dbcommon/type/float.cc",
        "dbcommon/src/dbcommon/type/type-modifier.h",
        "dbcommon/src/dbcommon/type/varlen.cc",
        "dbcommon/src/dbcommon/type/bool.cc",
        "dbcommon/src/dbcommon/type/float.h",
        # "dbcommon/src/dbcommon/type/type-util.cc",
        "dbcommon/src/dbcommon/type/varlen.h",
        "dbcommon/src/dbcommon/hash/**/*.h",
        "dbcommon/src/dbcommon/hash/**/*.cc",
        "dbcommon/src/dbcommon/testutil/**/*.h",
        "dbcommon/src/dbcommon/testutil/**/*.cc",
        "dbcommon/build/codegen/src/**/*.cc",
        "dbcommon/build/codegen/src/**/*.h",
    ]),
    hdrs = glob([
        "dbcommon/src/dbcommon/**/*.h",
    ]),
    includes = [
        "dbcommon/src",
        "dbcommon/build/codegen/src",
    ],
    deps = [
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
        "univplan/build/src/univplan/**/*.h",
        "univplan/build/src/univplan/**/*.cc",
        "univplan/src/univplan/**/*.h",
        "univplan/src/univplan/**/*.cc",
    ]),
    hdrs = glob([
        "univplan/src/univplan/**/*.h",
    ]),
    includes = [
        "univplan/src",
        "univplan/build/src",
    ],
    deps = [
        # ":protos_plan_expr_cc",
        # ":protos_plan_catalog_cc",
        # ":protos_plan_cc",
        ":dbcommon",
        "@protobuf_archive//:protobuf",
    ],
    copts = HORNET_COPTS,
    visibility = ["//visibility:public"],
)

# tf_proto_library_cc(
#     name = "protos_plan",
#     srcs = [
#         "univplan/src/univplan/proto/universal-plan.proto",
#         "univplan/src/univplan/proto/universal-plan-expr.proto",
#         "univplan/src/univplan/proto/universal-plan-catalog.proto",
#     ],
#     include = "univplan/src",
#     default_header = True,
#     visibility = ["//visibility:public"],
# )

cc_library(
    name = "rpc",
    srcs = glob([
        "rpc/build/src/rpc/**/*.h",
        "rpc/build/src/rpc/**/*.cc",
        "rpc/src/rpc/**/*.h",
        "rpc/src/rpc/**/*.cc",
    ]),
    hdrs = glob([
        "rpc/src/rpc/**/*.h",
    ]),
    deps = [
        ":dbcommon",
        # ":protos_rpc_cc",
        "@protobuf_archive//:protobuf",
    ],
    includes =[
        "rpc/src",
        "rpc/build/src",
    ],
    copts = HORNET_COPTS,
    linkopts = [
        "-lpthread",
        "-lc++",
    ],
    visibility = ["//visibility:public"],
)

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
cc_library(
    name = "kv-client",
    srcs = glob([
        "kv/src/kv/client/**/*.h",
        "kv/src/kv/client/**/*.cc",
        "kv/src/kv/cwrapper/**/*.h",
        "kv/src/kv/cwrapper/**/*.cc",
    ]),
    hdrs = glob([
        "kv/src/kv/client/**/*.h",
        "kv/src/kv/cwrapper/**/*.h",
    ]),
    deps = [
        ":rpc",
        ":univplan",
        ":dbcommon",
        # ":protos_magma_cc",
        "@hornet_dep//:glog",
        # "@grpc//:grpc++",
        "@protobuf_archive//:protobuf",
    ],
    includes = [
        "kv/src"
    ],
    copts = HORNET_COPTS,
    linkopts = [
        "-lpthread",
        "-lc++",
    ],
    visibility = ["//visibility:public"],
)

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
        "storage/build/src/storage/**/*.h",
        "storage/build/src/storage/**/*.cc",
        "storage/src/storage/**/*.h",
        "storage/src/storage/**/*.cc",
    ]),
    hdrs = glob([
        "storage/src/storage/**/*.h",
    ]),
    deps = [
        ":kv-client",
        ":univplan",
        ":dbcommon",
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