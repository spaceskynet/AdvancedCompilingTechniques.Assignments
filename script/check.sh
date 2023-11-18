#!/usr/bin/env bash
WORK_DIR=$(cd `dirname $0`; cd ..; pwd)

cd $WORK_DIR/build && ctest