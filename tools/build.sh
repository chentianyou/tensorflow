#! /bin/bash

# build thirdparty
git clone -b tf-dependency https://github.com/oushu-io/thirdparty.git ~/thirdparty
cd ~/thirdparty && make build-all-tf

git clone -b tf-dependency https://github.com/oushu-io/libhdfs3.git ~/libhdfs3
cd ~/libhdfs3 && make

git clone -b tf-dependency https://github.com/oushu-io/hornet.git ~/hornet
cd ~/hornet
make release

git clone -b oushu-master https://github.com/chentianyou/tensorflow.git
# HORNET_PATH=`pwd`
# sed -i 's?import "univplan/proto/universal-plan-catalog.proto";?import "univplan/src/univplan/proto/universal-plan-catalog.proto";?g' $HORNET_PATH/univplan/src/univplan/proto/universal-plan.proto
# sed -i 's?import "univplan/proto/universal-plan-expr.proto";?import "univplan/src/univplan/proto/universal-plan-expr.proto";?g' $HORNET_PATH/univplan/src/univplan/proto/universal-plan.proto

cd ~/tensorflow
parameter="//tensorflow/tools/pip_package:build_pip_package --compilation_mode=dbg --sandbox_debug"
if [ "$1" != "debug" ];then
    parameter="--config=opt //tensorflow/tools/pip_package:build_pip_package"
fi

bazel build $parameter
if [ $? != 0 ];then
    exit 1
fi