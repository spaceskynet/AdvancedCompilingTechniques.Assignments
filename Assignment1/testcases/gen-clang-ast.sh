#!/bin/bash

# 源代码目录
WORK_DIR=$(cd `dirname $0`; pwd)

if [ $# -lt 1 ]; then
    echo "Error: At least one argument is required."
    exit 1
fi

src_dir="$WORK_DIR/$1"
# 目标目录，用于存储生成的 AST 文件
ast_dir="$src_dir-ast"

# 确保目标目录存在
mkdir -p "$ast_dir"

# 遍历源代码目录中的所有 C 文件
find "$src_dir" -type f -name "*.c" | while read -r source_file; do
  # 构建目标 AST 文件路径
  source_filename=$(basename "$source_file")
  ast_file="$ast_dir/${source_filename%.c}.ast"

  # 使用 clang-check 生成 AST，去除着色
  clang-check -ast-dump "$source_file" --extra-arg="-fno-color-diagnostics" -- > "$ast_file"

  if [ $? -eq 0 ]; then
    echo "Generated AST for $source_file and saved as $ast_file"
  else
    echo "Failed to generate AST for $source_file"
  fi
done
