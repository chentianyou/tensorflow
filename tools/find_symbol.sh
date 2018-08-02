#!/bin/bash

FIND_DIR=$1
FIND_STR=$3
FIND_FILE=$2
#-o -name "*.so"
for name in `find $FIND_DIR -name "$FIND_FILE"`;do
    nm $name | grep "$FIND_STR" 2>/dev/null
    if [[ $? != 1 ]];then
        echo $name
    fi
done