# https://docs.bazel.build/versions/master/be/c-cpp.html#cc_library

load(
    "@org_tensorflow//tensorflow/core:platform/default/build_config.bzl",
    "tf_proto_library",
    "tf_proto_library_cc",
)

load(
    "@org_tensorflow//tensorflow:tensorflow.bzl",
    "if_linux_x86_64",
    "if_darwin",
)
cc_library(
    name = "hdfs3",
    srcs = glob([
        "src/**/*.cpp",
        "src/**/*.h",
    ]) + if_darwin([":platform_darwin"]) + if_linux_x86_64([":platform_linux"]),
    hdrs = glob([
        "BlockLocation.h",
        "DirectoryIterator.h",
        "FileStatus.h",
        "FileSystem.h",
        "FileSystemStats.h",
        "hdfs.h",
        "InputStream.h",
        "OutputStream.h",
        "Permission.h",
        "Exception.h",
        "XmlConfig.h",
    ]),
    includes = [
        "src/proto-tf",
        "src/common",
        "src/client",
        "build/src",
        "src",
        "darwin",
        "linux",
    ],
    deps = [
        "@boringssl//:crypto",
        ":protos_all_cc_impl",
        "@hornet_dep//:libxml2",
        "@hornet_dep//:gsasl",
        "@curl//:curl"
    ],
    copts = [
        "-Dprivate=public",
        "-Dprotected=public",
    ], 
    linkopts = [
        "-lpthread",
        "-lc++",
        "-lkrb5",
    ] + if_linux_x86_64([ "-luuid"]),
    visibility = ["//visibility:public"],
)

HDFS_PROTO_SRCS=[
    "src/proto-tf/ProtobufRpcEngine.proto",
    "src/proto-tf/datatransfer.proto",
    "src/proto-tf/RpcHeader.proto",
    "src/proto-tf/IpcConnectionContext.proto",
    "src/proto-tf/ClientNamenodeProtocol.proto",
    "src/proto-tf/ClientDatanodeProtocol.proto",
    "src/proto-tf/Security.proto",
    "src/proto-tf/hdfs.proto",
    "src/proto-tf/encryption.proto",
]

tf_proto_library(
    name = "protos_all",
    srcs = HDFS_PROTO_SRCS,
    cc_api_version = 2,
    default_header = True,
    visibility = ["//visibility:public"],
)

genrule(
    name = 'platform_linux',
    outs = [
        'linux/platform.h',
    ],
    cmd = r'''\
cat > $@ <<"EOF"
#define THREAD_LOCAL __thread
#define ATTRIBUTE_NORETURN __attribute__ ((noreturn))
#define ATTRIBUTE_NOINLINE __attribute__ ((noinline))

#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)

/* #undef LIBUNWIND_FOUND */
/* #undef HAVE_DLADDR */
#define OS_LINUX
/* #undef OS_MACOSX */
#define ENABLE_FRAME_POINTER
/* #undef HAVE_SYMBOLIZE */
/* #undef NEED_BOOST */
/* #undef STRERROR_R_RETURN_INT */
#define HAVE_STEADY_CLOCK
#define HAVE_NESTED_EXCEPTION
#define HAVE_BOOST_CHRONO
#define HAVE_STD_CHRONO
#define HAVE_BOOST_ATOMIC
#define HAVE_STD_ATOMIC

// defined by gcc
#if defined(__ELF__) && defined(OS_LINUX)
# define HAVE_SYMBOLIZE
#elif defined(OS_MACOSX) && defined(HAVE_DLADDR)
// Use dladdr to symbolize.
# define HAVE_SYMBOLIZE
#endif

#define STACK_LENGTH 64

EOF
''',
    )

genrule(
    name = 'platform_darwin',
    outs = [
        'darwin/platform.h',
    ],
    cmd = r'''\
cat > $@ <<"EOF"
#define THREAD_LOCAL __thread
#define ATTRIBUTE_NORETURN __attribute__ ((noreturn))
#define ATTRIBUTE_NOINLINE __attribute__ ((noinline))

#define GCC_VERSION (__GNUC__ * 10000 \
                     + __GNUC_MINOR__ * 100 \
                     + __GNUC_PATCHLEVEL__)

/* #undef LIBUNWIND_FOUND */
#define HAVE_DLADDR
/* #undef OS_LINUX */
#define OS_MACOSX
#define ENABLE_FRAME_POINTER
/* #undef HAVE_SYMBOLIZE */
/* #undef NEED_BOOST */
#define STRERROR_R_RETURN_INT
#define HAVE_STEADY_CLOCK
#define HAVE_NESTED_EXCEPTION
#define HAVE_BOOST_CHRONO
#define HAVE_STD_CHRONO
#define HAVE_BOOST_ATOMIC
#define HAVE_STD_ATOMIC

// defined by gcc
#if defined(__ELF__) && defined(OS_LINUX)
# define HAVE_SYMBOLIZE
#elif defined(OS_MACOSX) && defined(HAVE_DLADDR)
// Use dladdr to symbolize.
# define HAVE_SYMBOLIZE
#endif

#define STACK_LENGTH 64

EOF
''',
    )