# 5.sh
# 3210103283 朱镐哲
# lab2-5，同步与备份

#!/bin/bash

# 检查输入参数
if [ $# -ne 2 ]; then
  echo "Usage: $0 <source_directory> <destination_directory>"
  exit 1
fi

source_dir="$1"
destination_dir="$2"

function my_backup() {
    local s="$1" # 原目录路径
    local d="$2" # 目标目录路径

    # 递归且复制更新部分
    cp -ur "$s"/* "$d"/

    # 对于目标目录中的所有文件进行检查
    for f in "$d"/*; do
        # 如果是文件
        if [ -f "$f" ]; then
            file_name="$s/"
            file_name+=`basename "$f"`
            delete_name="$d/"
            delete_name+=`basename "$f"`
            # 如果未能在原目录中找到对应文件，则删除
            if [ ! -e $file_name ]; then
                rm "$delete_name"
                echo "$delete_name has been deleted!"
            fi
        # 如果是目录
        elif [ -d "$f" ]; then
            dir_name="$s/"
            dir_name+=`basename "$f"`
            delete_name="$d/"
            delete_name+=`basename "$f"`
            # 如果未能在原目录中找到对应文件，则删除
            if [ ! -e dir_name ]; then
                rm -r "$delete_name"
                echo "$delete_name has been deleted!"
            # 如果能够找到，则需要深入目录进行检查
            else
                my_backup "$s/$dir_name" "$f"
            fi
        fi
    done

    echo
}

function my_sync() {
    local one="$1" # 目录1路径
    local two="$2" # 目录2路径

    # 和备份一样
    my_backup "$one" "$two"
    # 若原路径比目标路径的文件旧，则会被更新
    my_backup "$two" "$one"
}

# 主程序
# 如果原目录不存在，那么报错退出
if [ ! -d "$source_dir" ]; then
    echo "The source directory is not valid."
    exit 1
fi

# 如果目标路径不存在，那么新建一个文件夹
if [ ! -d "$destination_dir" ]; then
    mkdir "$destination_dir"
    echo "mkdir $destination_dir"
fi

# 选择备份或者同步的功能
echo "Please choose the mode: "
echo "1.back up"
echo "2.synchronize"
read -p "Please input the number:" choice

case $choice in
    1 ) # 备份
        my_backup $source_dir $destination_dir
        echo "Finish back up!"
        ;;
    2 ) # 同步
        my_sync $source_dir $destination_dir
        echo "Finish synchronization!"
        ;;
    * )
        echo "Invalid choice!"
        exit 1
        ;;
esac

exit 0
