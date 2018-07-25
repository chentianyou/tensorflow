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

mv /opt/dependency/package/include/json ~/json
mv /opt/dependency/package/include/univplan ~/univplan
rm /opt/dependency/libhdfs3/build/src/*.pb.h
rm /opt/dependency/libhdfs3/build/src/*.pb.cc

sed -i 's#"univplan/proto/universal-plan#"univplan/src/univplan/proto/universal-plan#g' /opt/dependency/hornet/univplan/src/univplan/proto/universal-plan.proto

parameter="-c dbg -c opts //tensorflow/tools/pip_package:build_pip_package \
--compilation_mode=dbg \
--sandbox_debug"

env_str='LD_LIBRARY_PATH=/opt/dependency/package/lib:/usr/local/lib:/usr/local/lib64:/usr/lib64'

if [ "$1" = "release" ];then
    parameter="--config=opt //tensorflow/tools/pip_package:build_pip_package"
fi

bazel build --jobs=8 $parameter --verbose_failures
exit_cout=$?

mv ~/json /opt/dependency/package/include/json
mv ~/univplan /opt/dependency/package/include/univplan
sed -i 's#"univplan/src/univplan/proto/universal-plan#"univplan/proto/universal-plan#g' /opt/dependency/hornet/univplan/src/univplan/proto/universal-plan.proto

if [ $exit_cout != 0 ];then
    exit 1
fi

~/tensorflow/bazel-bin/tensorflow/tools/pip_package/build_pip_package /opt/dependency
sudo pip3.6 uninstall -y /opt/dependency/tensorflow-1.*
sudo pip3.6 install /opt/dependency/tensorflow-1.*

python3.6 ./test/test.py

