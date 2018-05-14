# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library
cc_library(
    name = "test",
    srcs = ["test.cc"],
    visibility = ["//visibility:public"],
)

# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_binary
cc_binary(
    name = "size_test",
    srcs = [
        "static_test.h",
        "size_test.cc"
    ],
    visibility = ["//visibility:public"],
)