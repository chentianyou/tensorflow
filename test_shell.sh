#! /bin/bash
# source ~/.anaconda_profile

parameter="//tensorflow/tools/pip_package:build_pip_package \
--compilation_mode=dbg \
--sandbox_debug"

if [ "$1" != "debug" ];then
    parameter="--config=opt //tensorflow/tools/pip_package:build_pip_package"
fi

bazel build $parameter
if [ $? != 0 ];then
    exit 1
fi

~/tensorflow/bazel-bin/tensorflow/tools/pip_package/build_pip_package /opt/dependency
sudo pip3.6 uninstall -y /opt/dependency/tensorflow-1.*
sudo pip3.6 install /opt/dependency/tensorflow-1.*

python3.6 ./test/test.py
