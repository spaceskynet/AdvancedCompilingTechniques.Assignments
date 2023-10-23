#!/usr/bin/env bash
WORK_DIR=$(cd `dirname $0`; pwd)

mkdir -p $WORK_DIR/build && cd $WORK_DIR/build
rm -rf ./* 
cmake .. -DCMAKE_BUILD_TYPE=Debug && cmake --build . --config Debug

