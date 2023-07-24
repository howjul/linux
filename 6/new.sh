if [ $# != 1 ]; then
if [ ! -d $1 ]; then
    echo "Error: $1 is not a valid directory."
    exit 1
fi
f=`find $1 -maxdepth 1`
regular_f_num=0
son_directory_num=0
executable_num=0
regular_f_space=0
for f in ${f[@]:1}
do
    if [ -f $f ]; then
        regular_f_num=$(( $regular_files_num+1 ))
        set `wc -c $f`
        regular_f_space=$(( $regular_files_space+$1 ))
    fi
    if [ -d $f ]; then
        son_directory_num=$(( $son_directory_num+1 ))
    fi
    if [ -x $f ]; then
        executable_num=$(( $executable_num+1 ))
    fi
done
echo "Number of regular f: $regular_files_num"
echo "Number of directories: $son_directory_num"
