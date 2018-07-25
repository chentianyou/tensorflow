#!/bin/bash

FIND_DIR=$1
FIND_STR=$2
#-o -name "*.so"
for name in `find $FIND_DIR -name "*.params"`;do
    grep "$FIND_STR" $name 2>/dev/null
    if [[ $? != 1 ]];then
        echo $name
        echo "------------------------------------"
    fi
done
