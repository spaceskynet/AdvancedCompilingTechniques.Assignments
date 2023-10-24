#!/usr/bin/env bash

WORK_DIR=$(cd `dirname $0`; pwd)
TEMP_DIR=tmp

mkdir -p $WORK_DIR/$TEMP_DIR

testcase_src=$WORK_DIR/$1
testcase_name=$(basename $testcase_src)
testcase_elf="$WORK_DIR/$TEMP_DIR/${testcase_name%.c}.out"
ast_interpreter=$WORK_DIR/build/ast-interpreter

clang $WORK_DIR/testcases/extern_func.c $testcase_src -o $testcase_elf && chmod +x $testcase_elf

$testcase_elf >$TEMP_DIR/std.txt 2> /dev/null
$ast_interpreter "`cat $testcase_src`" >$TEMP_DIR/self.txt 2> /dev/null
if diff $TEMP_DIR/std.txt $TEMP_DIR/self.txt; then
    echo -e "\e[34m$testcase_name\e[0m: \e[32mAC\e[0m"
    rm -rf $WORK_DIR/$TEMP_DIR
else
    echo -e "\e[34m$testcase_name\e[0m: \e[31mWa\e[0m"
    exit 0
fi