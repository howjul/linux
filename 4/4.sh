# 4.sh
# 3210103283 朱镐哲
# lab2-4，判断回文字符串

#! /bin/bash

# 首先判断是否为一个字符串
if [ $# -ne 1 ]; then
    echo "Usage: $0 <string>"
    exit 1
fi

# 读取输入字符串
ori_str="$1"
echo "input: $ori_str"
clean_str=""

# delete_nonchar()：删除非字母的字符
function delete_nonchar() {
    local ori_str="$1"

    # 对字符串逐字符进行遍历
    for ((i=0;i<${#ori_str};i++)); do
        # 取出第i位进行检验
        char="${ori_str:$i:1}"
        # 如果是字母就放入clean_str中
        if [[ "$char" ==  [a-zA-Z] ]]; then
            clean_str+="$char"
        fi
    done

    # 输出清理后的字符串
    echo "cleaned string: $clean_str"
}

# judge()：用于判断是否回文
function judge() {
    # 读取待判断的字符串
    local str="$1"
    local rev_str=""

    # 倒过来读取输入字符串，并输出到rev_str中
    for((i=${#str}-1;i>=0;i--)); do
        rev_str+="${str:$i:1}"
    done

    # 输出倒转后的字符串
    echo "reversed string: $rev_str"

    # 比较倒转前后字符串，输出结果
    if [ "$str" == "$rev_str" ]; then
        echo "result: Yes!"
    else
        echo "result: No!"
    fi
}

delete_nonchar $ori_str # 调用delete_nonchar函数，删除非字母
judge $clean_str # 调用judge函数，对处理后的字符串进行判断
