#! /bin/bash
# source ~/.anaconda_profile
# build --cxxopt=-std=c++11
# build --host_cxxopt=-std=c++11
# build --cxxopt=-stdlib=libc++
# build --host_cxxopt=-stdlib=libc++
# build --linkopt=-L/usr/local/lib
# build --host_linkopt=-L/usr/local/lib
# build --linkopt=-lc++
# build --host_linkopt=-lc++
set -x
parameter="-s //tensorflow/tools/pip_package:build_pip_package \
--compilation_mode=dbg \
--sandbox_debug"

env_str='LD_LIBRARY_PATH=/opt/dependency/package/lib:/usr/local/lib:/usr/local/lib64:/usr/lib64'

if [ "$1" = "release" ];then
    parameter="-s --config=opt //tensorflow/tools/pip_package:build_pip_package"
fi

bazel build $parameter
if [ $? != 0 ];then
    exit 1
fi

~/tensorflow/bazel-bin/tensorflow/tools/pip_package/build_pip_package /opt/dependency
sudo pip3.6 uninstall -y /opt/dependency/tensorflow-1.*
sudo pip3.6 install /opt/dependency/tensorflow-1.*

python3.6 ./test/test.py
