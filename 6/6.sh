#!/bin/bash

. print.sh # 用于显示文本的相关函数
. arrow.sh # 用于处理上下左右移动的相关函数

# 检查参数是否正确
if [ $# -ne 1 ]; then
  echo "Usage: $0 <filename>"
  exit 1
fi

filename="$1"       # 文件名
content=""          # 内容
content_row=0       # 终端显示的第一行对应文件的行数       
content_total=0     # 文件的总行数
cursor_row=0        # 光标在终端的行数位置
cursor_col=0        # 光标在终端的列数位置
rows=`tput lines`   # 终端总行数
cols=`tput cols`    # 终端总列数

# 读取文件内容
. scan.sh # 生成两个数组，记录每行的起始字符地址和每行的字符个数
adivce="" # 显示在左下角的提示，类似vim中的提示

# 编辑器主循环
signal=0  # 退出信号
while true; do
  advice="1-插入模式 2-命令模式 3-保存退出 4-退出"
  ins=""
  flash # 刷新屏幕

  read -sn 1 input  # 读取单个按键输入，不需要按回车键

  case $input in
    1)
      advice="插入模式: Esc+Esc-退出 Esc+'d'-删除单个字符"
      flash
      # 对于行内的字符操作
      while true; do
        # 读取单个按键输入，不需要按回车键
        read -sn 1 input

        if [ "$input" == $'\e' ]; then
          # 如果读到esc，则有三种情况，这三种情况都在：
          # 1. +Esc 退出插入模式
          # 2. +d 删除当前光标所在位置的字符
          # 3. 上下左右箭头 移动光标
          handle_mode
          # signal为1也就是按下两次Esc时
          if [ "$signal" == 1 ]; then
            signal=0
            break
          fi
        else
          # 否则就是正常的插入
          cursor_col=$((cursor_col + 1)) # 光标位置右移一位
          # 更新content
          old_content=$content
          content="${old_content:0:$(( ${line_charnumber[$((content_row + cursor_row))]} + content_row + cursor_row + cursor_col - 1 ))}"
          content+="$input"
          content+="${old_content:$(( ${line_charnumber[$((content_row + cursor_row))]} + content_row + cursor_row + cursor_col - 1 ))}"
          update_insert_linelen  # 更新行数数组
          update_insert_charnumber  # 更新首字母数组
          flash # 刷新屏幕
        fi
      done
      ;;
    2)
      # 命令模式
      advice="命令模式: Esc+Esc-退出 命令以'.'结尾"
      flash
      touch temp # 创建一个临时文件
      echo "$content" > "temp" # 把当前内容写入临时文件

      # 命令模式主循环
      while true; do
        # 读取单个按键输入，不需要按回车键
        read -sn 1 input

        if [ "$input" == $'\e' ]; then
          # 如果读到esc，则有两种情况：
          # 1. +Esc 退出模式
          # 2. 上下左右箭头 移动光标
          handle_mode
          # signal为1也就是按下两次Esc时
          if [ "$signal" == 1 ]; then
            signal=0
            break
          fi
        else
          # 否则就是正常命令
          ins=""
          # 把IFS设置成非空格，防止空格无法读入
          IFSbak=$IFS
          IFS="\n"
          # 读取命令       
          while [ "$input" != '.' ]; do
            ins="$ins$input"
            flash
            read -sn 1 input
          done
          # 恢复IFS
          IFS=$IFSbaks
          # 使用sed来修改temp文件
          sed -i "$ins" temp
          # 用temp文件来初始化数组
          read_again
          flash # 刷新屏幕
        fi          
      done

      rm temp # 删除temp文件
      ;;
    3)
      # 保存退出
      echo "$content" > "$filename"
      clear
      echo "文件已保存，退出编辑器。"
      exit 0
      ;;
    4)
      # 退出
      clear
      echo "退出编辑器。"
      exit 0
      ;;
    $'\e')
      # 处理上下左右箭头
      handle_arrow
      ;;  
    *)
      # 其他情况
      ;;
  esac
done
