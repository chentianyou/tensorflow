#! /bin/bash
# source ~/.anaconda_profile

parameter="//tensorflow/tools/pip_package:build_pip_package \
--compilation_mode=dbg \
--sandbox_debug"

if [ "$1" != "debug" ];then
    parameter="--config=opt //tensorflow/tools/pip_package:build_pip_package"
fi

COPTS="--cxxopt=-stdlib=libc++ --linkopt=-lc++"

# --cxxopt=-stdlib=libc++ \
# COPTS="--cxxopt=-stdlib=libc++"
# --verbose_failures
# GCC using libc++
#  bazel build -s --cxxopt=-nostdinc++ --cxxopt=-I/usr/local/include/c++/v1/ --linkopt=-nodefaultlibs --linkopt=-L/usr/local/lib --linkopt=-lc++ --linkopt=-lc++abi --linkopt=-lm --linkopt=-lc --linkopt=-lgcc_s --linkopt=-lgcc //test:size_test

bazel build -s  --toolchain_resolution_debug $COPTS $parameter
if [ $? != 0 ];then
    exit 1
fi

./bazel-bin/tensorflow/tools/pip_package/build_pip_package /opt/dependency
sudo pip3.6 uninstall -y /opt/dependency/tensorflow-1.*
sudo pip3.6 install /opt/dependency/tensorflow-1.*

python3.6 ./test/test.py
