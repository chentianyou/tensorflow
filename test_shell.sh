#! /bin/bash
# source ~/.anaconda_profile

parameter="//tensorflow/tools/pip_package:build_pip_package
--compilation_mode=dbg \
--sandbox_debug \
--action_env PATH \
--action_env LD_LIBRARY_PATH \
--action_env DYLD_LIBRARY_PATH"
if [ "$1" != "debug" ];then
    parameter="--config=opt \
    //tensorflow/tools/pip_package:build_pip_package \
    --action_env PATH \
    --action_env LD_LIBRARY_PATH \
    --action_env DYLD_LIBRARY_PATH"
fi

bazel build $parameter
if [ $? != 0 ];then
    exit 1
fi
/Users/chentianyou/dev/tensorflow/bazel-bin/tensorflow/tools/pip_package/build_pip_package ~/dev
pip uninstall -y ~/dev/tensorflow-1.7.0-cp36-cp36m-macosx_10_7_x86_64.whl
pip install ~/dev/tensorflow-1.7.0-cp36-cp36m-macosx_10_7_x86_64.whl