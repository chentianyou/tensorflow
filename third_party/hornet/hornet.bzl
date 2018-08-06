def cogapp_gen_prepare():
    native.genrule(
        name = 'gen_sh',
        outs = [
            'gen.sh',
        ],
        cmd =  r'''\
#!/bin/sh
cat > $@ <<"EOF"
#!/bin/sh
while [ $$# -gt 0 ];do
    outfile=$$1
    shift
    infile=$$1
    shift
    python -m cogapp -d -o $$outfile $$infile
done
EOF
''',
        )
    native.sh_binary(
        name = "cogapp_gen_sh",
        srcs = [
            ":gen_sh"
        ],
    )
    native.filegroup(
        name = "cogapp_dep",
        srcs = ["dbcommon/src/dbcommon/python/code_generator.py"],
    )
    native.genrule(
        name = 'tft_gen_sh',
        outs = [
            'tft_gen.sh',
        ],
        cmd =  r'''\
#!/bin/sh
cat > $@ <<"EOF"
#!/bin/sh
thrift_bin=`which thrift`
if [ "$$thrift_bin" = "" ];then
    echo "Not found thrift!"
    exit 1
fi
$$thrift_bin $$@
EOF
''',
        )
    native.sh_binary(
        name = "thrift_gen_sh",
        srcs = [
            ":tft_gen_sh"
        ],
    )

def _cogapp_gen_impl(ctx):
    srcs = ctx.files.srcs
    outs = ctx.outputs.outs
    args = []
    for i in range(len(outs)):
        args.append(outs[i].path)
        args.append(srcs[i].path)
    ctx.action(
        inputs=srcs + ctx.files.deps,
        outputs=outs,
        arguments=args,
        executable= ctx.executable.cogapp,
        mnemonic="CogappGenerate",
    )

_cogapp_gen = rule(
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "deps": attr.label(allow_files = True),
        "cogapp": attr.label(
            cfg = "host",
            executable = True,
            mandatory = True,
        ),
        "outs":attr.output_list(),
    },
    output_to_genfiles = True,
    implementation = _cogapp_gen_impl,
)

def cogapp_gen(
    name,
    srcs=[]):
    outputs = []
    for f in srcs:
        if f.endswith(".h"):
            outputs.append(f[:-2] + ".cg.h")
        if f.endswith(".cc"):
            outputs.append(f[:-3] + ".cg.cc")
    _cogapp_gen(
        name=name,
        srcs=srcs,
        deps=":cogapp_dep",
        cogapp=":cogapp_gen_sh",
        outs=outputs)

def _thrift_gen_impl(ctx):
    srcs = ctx.files.srcs
    if len(srcs) != 1:
        fail("Exactly one SWIG source file label must be specified.", "srcs")
    src = srcs[0]
    deps = ctx.files.deps
    outs = ctx.outputs.outs
    for out in outs:
        index = out.path.rindex("/")
        outdir = out.path[:index]
    args = ["--gen", "cpp", "--recurse", "-out", outdir]
    for i in deps:
        args.append("-I")
        index = i.path.rindex("/")
        idir = i.path[:index]
        args.append(idir)
    for f in srcs:
        args.append(f.path)
    ctx.action(
        inputs=srcs,
        outputs=outs,
        arguments=args,
        executable= ctx.executable.thrift,
        use_default_shell_env=True,
        mnemonic="ThriftGen",
    )

_thrift_gen = rule(
    attrs = {
        "srcs": attr.label_list(allow_files = True),
        "deps": attr.label_list(allow_files = True),
        "thrift": attr.label(
            cfg = "host",
            executable = True,
            mandatory = True,
        ),
        "outs": attr.output_list(),
    },
    output_to_genfiles = True,
    implementation = _thrift_gen_impl,
)

def thrift_gen(
    name):
    srcs = [
        "dbcommon/src/dbcommon/filesystem/hive/hive_metastore.thrift",
    ]
    deps = [
        "dbcommon/src/dbcommon/filesystem/hive/fb303.thrift",
    ]
    outputs = []
    for f in srcs:
        if f.endswith(".thrift"):
            outputs.append(f[:-7] + "_types.h")
            outputs.append(f[:-7] + "_types.cpp")
            outputs.append(f[:-7] + "_constants.h")
            outputs.append(f[:-7] + "_constants.cpp")
    for f in deps:
        if f.endswith(".thrift"):
            outputs.append(f[:-7] + "_types.h")
            outputs.append(f[:-7] + "_types.cpp")
            outputs.append(f[:-7] + "_constants.h")
            outputs.append(f[:-7] + "_constants.cpp")
    outputs.append("dbcommon/src/dbcommon/filesystem/hive/FacebookService.h")
    outputs.append("dbcommon/src/dbcommon/filesystem/hive/FacebookService.cpp")
    outputs.append("dbcommon/src/dbcommon/filesystem/hive/ThriftHiveMetastore.h")
    outputs.append("dbcommon/src/dbcommon/filesystem/hive/ThriftHiveMetastore.cpp")
    _thrift_gen(
        name=name,
        srcs=srcs,
        deps=deps,
        thrift=":thrift_gen_sh",
        outs=outputs)