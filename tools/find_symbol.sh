#!/bin/bash

FIND_DIR=$1
FIND_STR=$2
#-o -name "*.so"
for name in `find $FIND_DIR -name "*.o"`;do
    nm $name | grep "$FIND_STR" 2>/dev/null
    if [[ $? != 1 ]];then
        echo $name
    fi
done
