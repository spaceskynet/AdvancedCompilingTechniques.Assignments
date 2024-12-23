#!/usr/bin/env bash

WORK_DIR=$(cd `dirname $0`; pwd)
TEMP_DIR=tmp

if [ $# -lt 1 ]; then
    echo -e "\e[31m[Error]\e[0m At least one argument is required."
    exit 1
fi

# 设置目标测试用例目录
directory="$WORK_DIR/testcases/$1"

if ! [[ -d $directory ]]; then
    echo -e "\e[31m[Error]\e[0m No such directory for testcases!"
    exit 1
fi

# 遍历目录中的匹配文件
echo -e "\e[32m[Info]\e[0m Check Testcases in $directory: "
echo -e "\e[33mStart checking...\e[0m"
return_status=0
find "$directory" -type f -regex '.*test[0-9][0-9]\.c' | while read -r file; do
  relative_path=$(echo "$file" | sed "s|^$WORK_DIR||")
  ./check.sh $relative_path
  set $return_status=$?
done

echo -e "\e[32m[Info]\e[0m All done!"

