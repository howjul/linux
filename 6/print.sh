# 移动光标到指定位置
function move_cursor() {
  tput cup $cursor_row $cursor_col
}

# 定义分页函数
function page_content() {
  # 计算起始行号
  if [ $cursor_row -eq 0 ]; then
    start_line=$((content_row + 1))
  elif [ $cursor_row -eq $((rows - 2)) ]; then
    start_line=$((content_row + 1))
  fi
  # 如果起始行号小于1，则设为1
  if [ $start_line -lt 1 ]; then
    start_line=1
  fi

  # 计算结束行号，留一个空白行
  end_line=$((start_line + rows - 2))
  if [ $end_line -gt $content_total ]; then
    end_line=$content_total
  fi

  # 显示当前页的内容
  old_col=$((cursor_col))
  cursor_col=0
  move_cursor
  echo "$content" | sed -n "${start_line},${end_line}p"
  cursor_col=$((old_col))
  move_cursor
}

# 在底部显示提示
function suggestion {
  # 保存原来的光标位置
  local old_col=$((cursor_col))
  local old_row=$((cursor_row))
  # 把光标移动到屏幕最后一行
  cursor_col=0
  cursor_row=$((rows-1))
  move_cursor
  # 打印出提示信息
  echo -n "$advice $((content_row + old_row)),$old_col $input $ins"
  # 恢复光标位置
  cursor_col=$((old_col))
  cursor_row=$((old_row))
  move_cursor
}

# # 清屏并绘制界面
function flash {
  clear
  move_cursor
  page_content
  suggestion
}