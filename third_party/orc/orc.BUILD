exports_files(["LICENSE"])

cc_library(
    name = "orc_library",
    srcs = glob([
        "lib/libstorage.dylib",
        "lib/libdbcommon.dylib",
        "lib/libexecutor.dylib",
        "lib/libkv-client.dylib",
    ]),
    hdrs = glob([
        "include/**",
    ]),
    linkopts = [
        "-lpthread",
    ],
    includes = ["include"],
    visibility = ["//visibility:public"],
)

#cc_library(
#    name = "orc_hearders",
#    srcs = glob([
#        "include/**",
#    ]),
#    includes = ["include"],
#    visibility = ["//visibility:public"],
#)
#
#file_group(
#    name = "libstorage.dylib",
#    srcs = [
#        "lib/libstorage.dylib",
#    ],
#    visibility = ["//visibility:public"],
#)