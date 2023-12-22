#!/bin/bash

# 源代码目录
WORK_DIR=$(cd `dirname $0`; cd ..; pwd)

if [ $# -lt 1 ]; then
    echo "Error: At least one argument is required."
    exit 1
fi

src_dir="$WORK_DIR/testcases/$1"
# 目标目录，用于存储生成的 ll 文件和 CallGraph 等 png
ll_dir="$src_dir"

# 确保目标目录存在
mkdir -p "$ll_dir"

# 遍历源代码目录中的所有 C 文件
find "$src_dir" -type f -name "*.c" | while read -r source_file; do
  # 构建目标 ll 文件路径
  source_filename=$(basename "$source_file")
  ll_file="$ll_dir/${source_filename%.c}.ll"
  ll_m2r_file="$ll_dir/${source_filename%.c}.m2r.ll"
  png_file="$ll_dir/${source_filename%.c}.png"
  callgraph_png_file="$ll_dir/${source_filename%.c}.callgraph.png"

  # 使用 clang 生成 ll, 并处理为 SSA
  clang -Xclang -disable-O0-optnone -O0 -g3 -S -emit-llvm "$source_file" -o "$ll_file"
  opt -S -mem2reg -o "$ll_m2r_file" "$ll_file"
  opt -dot-cfg "$ll_m2r_file" > /dev/null
  opt -dot-callgraph "$ll_m2r_file" > /dev/null

  # 生成 png 图片，需要提前安装 graphviz
  # dot -Tpng -o "$png_file" .moo.dot
  dot -Tpng -o "$callgraph_png_file" callgraph.dot
  rm -f .*.dot
  rm -f callgraph.dot

  if [ $? -eq 0 ]; then
    echo "Generated ll for $source_file and saved as $ll_file"
  else
    echo "Failed to generate ll for $source_file"
  fi
done
