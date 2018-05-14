#!/bin/bash

if [ "$1" == "" ];then
    echo "specify a directory."
    exit 1
fi

if [ "$2" == "" ];then
    echo "specify a search string"
    exit 1
fi

DIR=`cd $1 && pwd`
STR=$2

for i in `find -L $DIR -name "*.[o|a|so]"`
do
    nm "$i" | grep "$STR"
    if [ $? = 0 ];then
        echo $i
    fi
done

exit 0