#!/usr/bin/env bash

if ! [ $# -eq 1 ]; then
    echo -e "\e[31m[Error]\e[0m Only accept one required argument: ./ast-interpreter.sh <source>.c`."
    exit 1
fi

./build/ast-interpreter "`cat $1`" -d