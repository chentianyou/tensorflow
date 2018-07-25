exports_files(["LICENSE"])

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
    includes = ["include/libxml2"],
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

# thrift
cc_library(
    name = "thrift",
    srcs = glob([
        "lib/libthrift.a"
    ]),
    hdrs = glob([
        "include/thrift/**"
    ]),
    includes = ["include"],
    visibility = ["//visibility:public"],
)