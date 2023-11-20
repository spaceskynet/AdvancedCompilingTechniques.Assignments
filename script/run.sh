#!/usr/bin/env bash
WORK_DIR=$(cd `dirname $0`; cd ..; pwd)

cd $WORK_DIR

if ! [ $# -eq 2 ]; then
    echo "Error: Only two argument are required.\nUsage: ./run.sh <self/class> <num>"
    exit 1
fi

testcase="testcases/$1/test$2.bc"

if ! [ -e $testcase ]; then
    echo "Error: no such testcase: $testcase"
    exit 1
fi

./build/assignment3 $testcase