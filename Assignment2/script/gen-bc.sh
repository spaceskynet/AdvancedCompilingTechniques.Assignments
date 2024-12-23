#!/bin/bash

# 源代码目录
WORK_DIR=$(cd `dirname $0`; cd ..; pwd)

if [ $# -lt 1 ]; then
    echo "Error: At least one argument is required."
    exit 1
fi

src_dir="$WORK_DIR/testcases/$1"
# 目标目录，用于存储生成的 bc 文件
bc_dir="$src_dir"

# 确保目标目录存在
mkdir -p "$bc_dir"

# 遍历源代码目录中的所有 C 文件
find "$src_dir" -type f -name "*.c" | while read -r source_file; do
  # 构建目标 bc 文件路径
  source_filename=$(basename "$source_file")
  bc_file="$bc_dir/${source_filename%.c}.bc"

  # 使用 clang 生成 bc
  clang -c -emit-llvm "$source_file" -o "$bc_file" -O0 -g3

  if [ $? -eq 0 ]; then
    echo "Generated bc for $source_file and saved as $bc_file"
  else
    echo "Failed to generate bc for $source_file"
  fi
done
