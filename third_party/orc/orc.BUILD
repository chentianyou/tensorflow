exports_files(["LICENSE"])
# cc_library(
#     name = "orc_library",
#     srcs = glob([
#         "lib/libstorage.so",
#         "lib/libkv-client.so",
#         "lib/libdbcommon.so",
#     ]),
#     hdrs = glob([
#         "include/dbcommon/**",
#         "include/storage/**",
#     ]),
#     linkopts = [
#         "-lpthread",
#     ],
#     includes = ["include"],
#     visibility = ["//visibility:public"],
# )

cc_library(
    name = "orc_library",
    deps = [
        ":storage"
    ],
    linkopts = [
        "-lpthread",
        "-lc++",
    ],
    visibility = ["//visibility:public"],
)

cc_library(
    name = "storage",
    srcs = glob([
        "lib/libstorage.a",
    ]),
    hdrs = glob([
        "include/storage/**",
    ]),
    deps = [
        ":magma-client",
        ":univplan",
        ":dbcommon",
        ":hdfs3",
        ":lz4",
        ":glog",
        ":iconv",
        "@snappy",
        "@zlib_archive//:zlib",
        "@protobuf_archive//:protobuf",
    ],
    includes = ["include"],
    linkopts = [
        "-lpthread",
        "-lc++",
    ],
    visibility = ["//visibility:public"],
)

#magma-clien
cc_library(
    name = "magma-client",
    srcs = glob([
        "lib/libkv-client.a",
    ]),
    hdrs = glob([
        "include/kv/**",
    ]),
    deps = [
        ":univplan",
        ":dbcommon",
        ":glog",
        "@grpc//:grpc++",
        "@protobuf_archive//:protobuf",
    ],
    includes = ["include"],
    linkopts = [
        "-lpthread",
        "-lc++",
    ],
    visibility = ["//visibility:public"],
)

#univplan
cc_library(
    name = "univplan",
    srcs = glob([
        "lib/libunivplan.a",
    ]),
    hdrs = glob([
        "include/univplan/**",
    ]),
    deps = [
        ":dbcommon",
        ":glog",
        "@protobuf_archive//:protobuf",
    ],
    includes = ["include"],
    linkopts = [
        "-lpthread",
        "-lc++",
    ],
    visibility = ["//visibility:public"],
)

#dbcommon
cc_library(
    name = "dbcommon",
    srcs = glob([
        "lib/libdbcommon.a",
    ]),
    hdrs = glob([
        "include/dbcommon/**",
    ]),
    deps = [
        ":glog",
        ":iconv",
        ":hdfs3",
        "@zlib_archive//:zlib",
    ],
    includes = ["include"],
    linkopts = [
        "-lpthread",
        "-lc++",
    ],
    visibility = ["//visibility:public"],
)
##############################
# third_party
##############################

#lz4
cc_library(
    name = "lz4",
    srcs = glob([
        "lib/liblz4.a",
    ]),
    hdrs = glob([
        "include/lz4.h",
        "include/lz4frame.h",
        "include/lz4frame_static.h",
        "include/lz4hc.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)

#glog
cc_library(
    name = "glog",
    srcs = glob([
        "lib/libglog.a",
    ]),
    hdrs = glob([
        "include/glog/**",
    ]),
    deps = [
        ":gflags"
    ],
    includes = ["include"],
    linkopts = [
        "-lpthread",
        "-lc++",
    ],
    visibility = ["//visibility:public"],
)

#gflags
cc_library(
    name = "gflags",
    srcs = glob([
        "lib/libgflags.a",
    ]),
    hdrs = glob([
        "include/gflags/**",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)

#hdfs3
cc_library(
    name = "hdfs3",
    srcs = glob([
        "lib/libhdfs3.a",
    ]),
    hdrs = glob([
        "include/hdfs/**",
    ]),
    deps=[
        ":libxml2",
        ":gsasl",
        "@protobuf_archive//:protobuf",
    ],
    includes = ["include"],
    linkopts = [
        "-lpthread",
        "-luuid",
        "-lc++",
    ],
    visibility = ["//visibility:public"],
)

#libxml2
cc_library(
    name = "libxml2",
    srcs =glob([
        "lib/libxml2.a"
    ]),
    hdrs = glob([
        "include/libxml2/**"
    ]),
    deps=[
        ":iconv",
        "@zlib_archive//:zlib",
    ],
    includes = ["include"],
    linkopts = [
        "-lpthread",
    ],
    visibility = ["//visibility:public"],
)

#gsasl
cc_library(
    name = "gsasl",
    srcs = glob([
        "lib/libgsasl.a",
    ]),
    hdrs = glob([
        "include/gsasl-compat.h",
        "include/gsasl-mech.h",
        "include/gsasl.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)

#iconv
cc_library(
    name = "iconv",
    srcs = glob([
        "lib/libiconv.a",
    ]),
    hdrs = glob([
        "include/iconv.h",
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)