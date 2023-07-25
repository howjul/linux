# 读取文件并记录每一行的长度和每到数组
declare -a line_len
declare -a line_charnumber
line_charnumber[0]=0
last_index=0

# 以行为单位进行读取，并进行相应的数组初始化
while IFS= read -r line; do
  length=${#line}
  line_len+=($length)
  last_index=$((${#line_charnumber[@]}-1))
  line_charnumber+=($(( line_charnumber[last_index] + length )))
done < "$filename"

content=`cat "$filename"` # 将文件内容存入content中
content_total=`echo "$content" | wc -l` # 文本的总行数

# 在插入模式的追加中更新line_charnumber数组
function update_insert_charnumber() {
  for ((i=$((cursor_row+content_row+1)); i<${#line_len[@]}; i++)); do
    line_charnumber[i]=$((line_charnumber[i] + 1))
  done
}

# 在插入模式的追加中更新line_len数组
function update_insert_linelen() {
    line_len[$((cursor_row+content_row))]=$((line_len[cursor_row+content_row] + 1))
}

# 在插入模式的删除中更新line_charnumber数组
function update_charnumber_backspace() {
  for ((i=$((cursor_row+content_row+1)); i<${#line_len[@]}; i++)); do
    line_charnumber[i]=$((line_charnumber[i] - 1))
  done
}

# 在插入模式的删除中更新line_len数组
function update_linelen_backspace() {
  line_len[cursor_row+content_row]=$((line_len[cursor_row+content_row] - 1))
}

# 在命令模式中，再次生成数组
function read_again() {
  unset line_len
  unset line_charnumber
  line_charnumber[0]=0
  last_index=0

  while IFS= read -r line; do
    length=${#line}
    line_len+=($length)
    last_index=$((${#line_charnumber[@]}-1))
    line_charnumber+=($(( line_charnumber[last_index] + length )))
  done < "temp"

  content=`cat temp` # 将文件内容存入content中
  content_total=`echo "$content" | wc -l` # 文本的总行数
}