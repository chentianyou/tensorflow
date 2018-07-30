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

plamtform=`go env GOOS`
TF_DEP=/opt/tf-dependency
TF_DEP_PKG=${TF_DEP}/package
DEP_ROOT=/opt/dependency
DEP_PKG=${DEP_ROOT}/package

if [ ! -d ${TF_DEP} ];then
    mkdir -p ${TF_DEP}
fi

echo "Delete TF dependency"
rm -rf ${TF_DEP}/*
mkdir -p ${TF_DEP_PKG}/include
mkdir -p ${TF_DEP_PKG}/lib
echo "Done"

echo "Copy TF dependency"
echo "Copy lz4"
cp -r ${DEP_PKG}/include/lz4*.h ${TF_DEP_PKG}/include
cp -r ${DEP_PKG}/lib/liblz4.a ${TF_DEP_PKG}/lib

echo "Copy glog"
cp -r ${DEP_PKG}/include/glog ${TF_DEP_PKG}/include/glog
cp -r ${DEP_PKG}/lib/libglog.a ${TF_DEP_PKG}/lib


echo "Copy gflags"
cp -r ${DEP_PKG}/include/gflags ${TF_DEP_PKG}/include/gflags
cp -r ${DEP_PKG}/lib/libgflags.a ${TF_DEP_PKG}/lib

echo "Copy libxml2"
cp -r ${DEP_PKG}/include/libxml2 ${TF_DEP_PKG}/include/libxml2
cp -r ${DEP_PKG}/lib/libxml2.a ${TF_DEP_PKG}/lib

echo "Copy gsasl"
cp -r ${DEP_PKG}/include/gsasl*.h ${TF_DEP_PKG}/include
cp -r ${DEP_PKG}/lib/libgsasl.a ${TF_DEP_PKG}/lib

echo "Copy iconv"
cp -r ${DEP_PKG}/include/iconv.h ${TF_DEP_PKG}/include
cp -r ${DEP_PKG}/lib/libiconv.a ${TF_DEP_PKG}/lib

echo "Copy thrift"
cp -r ${DEP_PKG}/include/thrift ${TF_DEP_PKG}/include/thrift
cp -r ${DEP_PKG}/lib/libthrift.a ${TF_DEP_PKG}/lib

echo "Copy hdfs3"
cp -r ${DEP_ROOT}/libhdfs3 ${TF_DEP}/libhdfs3
rm ${TF_DEP}/libhdfs3/build/src/*.pb.h
rm ${TF_DEP}/libhdfs3/build/src/*.pb.cc

echo "Get hornet"
cp -r ${DEP_ROOT}/hornet ${TF_DEP}/hornet
find ${TF_DEP}/hornet -name "*.pb.*" | xargs rm -f
echo "Done"

if [ "$plamtform" = "darwin" ];then
    sed -i '' 's#"univplan/proto/universal-plan#"univplan/src/univplan/proto/universal-plan#g' ${TF_DEP}/hornet/univplan/src/univplan/proto/universal-plan.proto
else
    sed -i 's#"univplan/proto/universal-plan#"univplan/src/univplan/proto/universal-plan#g' ${TF_DEP}/hornet/univplan/src/univplan/proto/universal-plan.proto
fi

parameter="//tensorflow/tools/pip_package:build_pip_package \
--compilation_mode=dbg \
--sandbox_debug"

env_str='LD_LIBRARY_PATH=/opt/dependency/package/lib:/usr/local/lib:/usr/local/lib64:/usr/lib64'

if [ "$1" = "release" ];then
    parameter="--config=opt //tensorflow/tools/pip_package:build_pip_package"
fi

bazel build --jobs=8 $parameter --verbose_failures

if [ $? != 0 ];then
    exit 1
fi

./bazel-bin/tensorflow/tools/pip_package/build_pip_package /opt/dependency
sudo pip3.6 uninstall -y /opt/dependency/tensorflow-1.*
sudo pip3.6 install /opt/dependency/tensorflow-1.*

python3.6 ./test/test.py

