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
