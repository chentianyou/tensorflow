package(default_visibility = ["//visibility:public"])

licenses(["notice"]) # BSD/MIT-like license (for lz4)

cc_library(
    name = "lz4",
    srcs = [
        "lib/lz4.h",
        "lib/lz4frame.h",
        "lib/lz4frame_static.h",
        "lib/lz4hc.h",
        "lib/xxhash.h",
        "lib/lz4.c",
        "lib/lz4frame.c",
        "lib/lz4hc.c",
        "lib/xxhash.c"
    ],
    hdrs = [
        "lib/lz4.h",
        "lib/lz4frame.h",
        "lib/lz4frame_static.h",
        "lib/lz4hc.h",
        "lib/xxhash.h",
        "lib/lz4.c",
    ],
    includes = ["lib"],
    visibility = ["//visibility:public"],
)