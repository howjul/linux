# 处理输入为箭头，移动光标
function handle_arrow {
  read -rsn1 input
  case "$input" in
    "[")
      # 读取第三个字符
      read -rsn1 input
      case "$input" in
        "A")
          # 上箭头
          if [ $cursor_row -ge 1 ]; then
            cursor_row=$((cursor_row - 1))
          else
            if [ $content_row -ge 1 ]; then
              content_row=$((content_row - 1))
            fi
          fi

          limit=${line_len[$((content_row + cursor_row))]}
          if [ $cursor_col -ge $((limit)) ]; then
            cursor_col=$((limit - 1))
          fi
          ;;
        "B")
          # 下箭头
          if [ $cursor_row -le $((rows - 3)) ]; then
            cursor_row=$((cursor_row + 1))
          else
            if [ $content_row -lt $((content_total - rows + 1)) ]; then
              content_row=$((content_row + 1))
            fi
          fi

          limit=${line_len[$((content_row + cursor_row))]}
          if [ $cursor_col -ge $((limit)) ]; then
            cursor_col=$((limit - 1))
          fi
          ;;
        "C")
          # 右箭头
          limit=${line_len[$((content_row + cursor_row))]}
          if [ $cursor_col -lt $((limit - 1)) ]; then
            cursor_col=$((cursor_col + 1))
          fi
          ;;
        "D")
          # 左箭头
          if [ $cursor_col -ge 1 ]; then
            cursor_col=$((cursor_col - 1))
          fi
          ;;
        *)
          ;;
      esac
      flash # 刷新屏幕
      ;;
    *)
      ;;
  esac
}

# 在插入模式中处理
# 如果读到esc，则有三种情况，这三种情况都在：
# 1. +Esc 退出插入模式
# 2. +d 删除当前光标所在位置的字符
# 3. 上下左右箭头 移动光标
function handle_mode {
  read -rsn1 input
  case "$input" in
    "[")
      # 读取第三个字符
      read -rsn1 input
      case "$input" in
        "A")
          # 上箭头
          if [ $cursor_row -ge 1 ]; then
            cursor_row=$((cursor_row - 1))
          else
            if [ $content_row -ge 1 ]; then
              content_row=$((content_row - 1))
            fi
          fi
          limit=${line_len[$((content_row + cursor_row))]}
          if [ $cursor_col -ge $((limit)) ]; then
            cursor_col=$((limit - 1))
          fi
          ;;
        "B")
          # 下箭头
          if [ $cursor_row -le $((rows - 3)) ]; then
            cursor_row=$((cursor_row + 1))
          else
            if [ $content_row -lt $((content_total - rows + 1)) ]; then
              content_row=$((content_row + 1))
            fi
          fi
          limit=${line_len[$((content_row + cursor_row))]}
          if [ $cursor_col -ge $((limit)) ]; then
            cursor_col=$((limit - 1))
          fi
          ;;
        "C")
          # 右箭头
          limit=${line_len[$((content_row + cursor_row))]}
          if [ $cursor_col -lt $((limit - 1)) ]; then
            cursor_col=$((cursor_col + 1))
          fi
          ;;
        "D")
          # 左箭头
          if [ $cursor_col -ge 1 ]; then
            cursor_col=$((cursor_col - 1))
          fi
          ;;
        *)
          ;;
      esac
      flash
      ;;
    'd')
      # 删除
      cursor_col=$((cursor_col - 1))  # 光标左移
      old_content=$content
      content="${old_content:0:$(( ${line_charnumber[cursor_row]} + cursor_row + cursor_col ))}"
      content+="${old_content:$(( ${line_charnumber[cursor_row]} + cursor_row + cursor_col + 1 ))}"
      update_linelen_backspace  # 更新行数数组
      update_charnumber_backspace  # 更新首字母数组
      flash
      ;;
    *)
      # 退出插入模式
      break
      ;;
  esac
}