# 3.sh
# 3210103283 朱镐哲
# lab2-3，判断回文字符串

#! /bin/bash

# 判断是否为一个参数
if [ $# != 1 ]; then
    echo "Usage: $0 <Directory>" # 输出使用方法
    exit 1
fi

# 判断是否为一个有效文件夹
if [ ! -d $1 ]; then
    echo "Error: $1 is not a valid directory."
    exit 1
fi

# 初始化files数组为给定目录下的各种文件（不包括子文件夹中的文件）
# files=`find $1` # 递归所有子目录的文件
files=`find $1 -maxdepth 1` # 只递归一层深度

# 初始化记数器
regular_files_num=0
son_directory_num=0
executable_num=0
regular_files_space=0

# 遍历数组中的每一个文件
for f in ${files[@]:1}
do
    # 如果是普通文件，则统计字节数和个数
    if [ -f $f ]; then
        regular_files_num=$(( $regular_files_num+1 ))
        set `wc -c $f`
        regular_files_space=$(( $regular_files_space+$1 ))
    fi
    # 如果是目录文件，则统计个数
    if [ -d $f ]; then
        son_directory_num=$(( $son_directory_num+1 ))
    fi
    # 如果是可执行文件，则统计个数
    if [ -x $f ]; then
        executable_num=$(( $executable_num+1 ))
    fi
done

# 输出四个记数器
echo "Number of regular files: $regular_files_num"
echo "Number of directories: $son_directory_num"
echo "Number of executable files: $executable_num"
echo "Total byte count of regular files: $regular_files_space"

exit 0
