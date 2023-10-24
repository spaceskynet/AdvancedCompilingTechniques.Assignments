#!/usr/bin/env bash

if [ $# -lt 1 ]; then
    echo -e "\e[31m[Error]\e[0m At least one argument is required."
    exit 1
fi

./build/ast-interpreter "`cat $1`" $2 $3