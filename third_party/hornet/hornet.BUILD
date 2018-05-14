# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library

load(
    "@org_tensorflow//tensorflow/core:platform/default/build_config.bzl",
    "tf_proto_library",
    "tf_proto_library_cc",
)

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
    visibility = ["//visibility:public"],
)

cc_library(
    name = "univplan",
    srcs = glob([
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
        ":protos_plan_cc_impl",
        ":dbcommon",
        "@protobuf_archive//:protobuf",
    ],
    visibility = ["//visibility:public"],
)

tf_proto_library_cc(
    name = "protos_plan",
    srcs = [
        "univplan/src/univplan/proto/universal-plan.proto",
        "univplan/src/univplan/proto/universal-plan-expr.proto",
        "univplan/src/univplan/proto/universal-plan-catalog.proto",
    ],
    include = "univplan/src",
    default_header = True,
    visibility = ["//visibility:public"],
)

cc_library(
    name = "rpc",
    srcs = glob([
        "rpc/src/rpc/**/*.h",
        "rpc/src/rpc/**/*.cc",
    ]),
    hdrs = glob([
        "rpc/src/rpc/**/*.h",
    ]),
    deps = [
        ":dbcommon",
        ":protos_rpc_cc_impl",
        "@protobuf_archive//:protobuf",
    ],
    includes =[
        "rpc/src",
        "rpc/build/src",
    ],
    visibility = ["//visibility:public"],
)

tf_proto_library_cc(
    name = "protos_rpc",
    srcs = [
        "rpc/src/rpc/proto/myrpc.proto",
    ],
    include = "rpc/src",
    default_header = True,
    cc_grpc_version = 1,
    visibility = ["//visibility:public"],
)

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
        ":protos_magma_cc_impl",
        "@hornet_dep//:glog",
        "@protobuf_archive//:protobuf",
    ],
    includes = [
        "kv/src",
    ],
    linkopts = [
        "-luuid",
    ],
    # alwayslink = 1,
    visibility = ["//visibility:public"],
)

MAGMA_PROTO_SRC = [
    "kv/src/kv/client/common/batchrpc/protos/batchrpc.proto",
    "kv/src/kv/client/common/coord/protos/kvservice.proto",
]

tf_proto_library_cc(
    name = "protos_magma",
    srcs = MAGMA_PROTO_SRC,
    include = "kv/src",
    default_header = True,
    cc_grpc_version = 1,
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
        ":kv-client",
        ":univplan",
        ":dbcommon",
        ":protos_orc_cc_impl",
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
    linkopts = [
        "-luuid",
    ],
    # alwayslink = 1,
    visibility = ["//visibility:public"],
)

tf_proto_library_cc(
    name = "protos_orc",
    srcs = [
        "storage/src/storage/format/orc/orc_proto.proto"
    ],
    include = "storage/src",
    default_header = True,
    visibility = ["//visibility:public"],
)