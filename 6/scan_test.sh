#!/bin/bash

# 提示用户输入文件名
read -p "请输入文件名: " filename

# 检查文件是否存在
if [ ! -f "$filename" ]; then
  echo "错误：文件不存在或不是一个普通文件。"
  exit 1
fi

# 读取文件并记录每一行的长度到数组
declare -a line_len
declare -a line_firchar
line_charnumber[0]=0
last_index=0

while IFS= read -r line; do
  length=${#line}
  line_len+=($length)
  last_index=$((${#line_charnumber[@]}-1))
  line_charnumber+=($(( line_charnumber[last_index] + length )))
done < "$filename"

echo $last_index

# 打印数组中的每个元素（每一行的长度）
echo "文件每一行的长度:"
for ((i=0; i<${#line_len[@]}; i++)); do
  echo "行 $((i)) 的长度: ${line_len[i]}"
  echo "行 $((i)) 的首字母: ${line_charnumber[i]}"
done
