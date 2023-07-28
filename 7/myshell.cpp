//myshell.cpp
//朱镐哲 3210103283

//头文件
#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <fstream>
#include <vector>
#include <dirent.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
using namespace std;

//全局变量
//环境变量
int shellpid;       //当前进程号
string hostname;    //主机
string username;    //用户名
string homepath;    //主目录路径
string pwd;         //当前路径
//参数
int argc;
string argv[1024];
//执行状态
int state;
//输出
string output;      //正常输出
string operror;     //错误信息


void initshell(int Argc, char *Argv[]);
void analyze(string cmd[], int argnum);
void printmessage(string mes1, string mes2, int cur_state);

void my_cd(string cmd[], int argnum);
void my_dir(string cmd[], int argnum);
void my_clr(string cmd[], int argnum);
void my_echo(string cmd[], int argnum);
void my_pwd(string cmd[], int argnum);
void my_exit(string cmd[], int argnum);
void my_time(string cmd[], int argnum);
void my_set(string cmd[], int argnum);
void my_help(string cmd[], int argnum);
void my_umask(string cmd[], int argnum);
void my_exec(string cmd[], int argnum);

int main(int Argc, char *Argv[]){
    initshell(Argc, Argv);
    int flag = 1;
    while(flag){
        //打印提示
        //如果路径中包含主目录，则替换成～
        string cur_path = pwd;
        size_t pos = pwd.find(homepath);
        if(pos != string::npos){
            cur_path.erase(pos, homepath.length());
            cur_path = "~" + cur_path;
        }
        printf("\033[36m%s@%s: \033[33m%s$ \033[?25h", username.c_str(), hostname.c_str(), cur_path.c_str());

        //存储命令
        string command;
        getline(cin, command);  //读入以换行为末尾的指令
        istringstream input(command);
        vector<string> cmd;
        string word;
        while(input >> word) cmd.push_back(word); //把指令按空格分成若干字符串

        //解析命令
        state = 0;
        analyze(cmd.data(), cmd.size());
    }
    return 0;
}

void initshell(int Argc, char * Argv[]){
    //获得当前进程号
    shellpid = getpid();

    //获得主机名字
    char buf[512];
    hostname = gethostname(buf, 512);
    hostname = buf;

    username = getenv("USER");  //获得当前用户名
    homepath = getenv("HOME");  //获得主路径
    pwd = getenv("PWD");  //获得当前路径

    //参数复制
    argc = Argc;
    for(int i = 0; i < Argc; i++){
        argv[i] = Argv[i];
    }

    //如果有两个参数，那就是正常进入
    if(Argc == 2){

    }

    return;
}

void analyze(string cmd[], int argnum){
    if(cmd[0] == "cd"){
        my_cd(cmd + 1, argnum - 1);
    }else if(cmd[0] == "dir"){
        my_dir(cmd + 1, argnum - 1);
    }else if(cmd[0] == "clr"){
        my_clr(cmd + 1, argnum - 1);
    }else if(cmd[0] == "echo"){
        my_echo(cmd + 1, argnum - 1);
    }else if(cmd[0] == "pwd"){
        my_pwd(cmd + 1, argnum - 1);
    }else if(cmd[0] == "exit"){
        my_exit(cmd + 1, argnum - 1);
    }else if(cmd[0] == "time"){
        my_time(cmd + 1, argnum - 1);
    }else if(cmd[0] == "set"){
        my_set(cmd + 1, argnum - 1);
    }else if(cmd[0] == "help"){
        my_help(cmd + 1, argnum - 1);
    }else if(cmd[0] == "umask"){
        my_umask(cmd + 1, argnum - 1);
    }else if(cmd[0] == "exec"){
        my_exec(cmd + 1, argnum - 1);
    }else{
        printmessage("", "Error! Command not found.\n", 1);
    }
    return ;
}

void printmessage(string mes1, string mes2, int cur_state){
    output = mes1;
    operror = mes2;
    state = cur_state;
    printf("%s%s", output.c_str(), operror.c_str());
    return;
}

void my_cd(string cmd[], int argnum){
    if (argnum == 0){
        printmessage(pwd.c_str(), "", 0);
    }
    //如果参数个数不为1，直接报错退出
    if (argnum != 1){
        printmessage("", "Error! Usage: cd <path>\n", 1);
        return;
    }
    //如果参数个数为1
    string path = cmd[0];
    //如果要回到主目录，需要预处理
    if (path == "~") path = homepath;
    if (chdir(path.c_str()) == 0) {
        //如果跳转成功
        char path_changed[1024];
        getcwd(path_changed, 1024);
        pwd = path_changed;
        //更新环境变量
        setenv("PWD", pwd.c_str(), 1);
    } else {
        //如果跳转失败
        printmessage("", "Error! Cannot change directory.\n", 1);
        return;
    }

    state = 0;
    return;
}

void my_dir(string cmd[], int argnum){
    DIR *dir;
    struct dirent *ptr;

    //检查参数
    if(argnum == 0){
        //如果无参数就打印当前目录
        dir = opendir(pwd.c_str());
    }else if(argnum == 1){
        //如果有一个参数那就打印参数的目录内容
        string target;

        //如果是一些特殊符号，需要进行预处理
        size_t pos;
        if((pos = cmd[0].find("../")) != string::npos){
            //获取目标文件夹
            chdir(cmd[0].c_str());
            char path_changed[1024];
            getcwd(path_changed, 1024);
            chdir(pwd.c_str());
            target = path_changed;
        }else if((pos = cmd[0].find("./")) != string::npos){
            target = cmd[0];
            target.erase(pos, 1);
            target = pwd + target;
        }else if((pos = cmd[0].find("~/")) != string::npos){
            target = cmd[0];
            target.erase(pos, 1);
            target = homepath + target;
        }else if(cmd[0] == "."){
            target = pwd;
        }else if(cmd[0] == ".."){
            //获取父文件夹
            chdir(cmd[0].c_str());
            char path_changed[1024];
            getcwd(path_changed, 1024);
            chdir(pwd.c_str());
            string parent = path_changed;
            target = parent;
        }else if(cmd[0] == "~"){
            target = homepath;
        }

        //调用opendir打开文件夹
        dir = opendir(target.c_str());
    }else{
        //参数过多，报错
        printmessage("", "Error! No more then one parameter.\n", 1);
        return;
    }

    //检查路径
    if(dir == NULL){
        printmessage("", "Error! Cannot open directory.\n", 1);
        return;
    }

    //打印内容
    stringstream ss;
    while((ptr = readdir(dir)) != NULL){
        ss << ptr->d_name << " ";
    }
    ss << endl;
    printmessage(ss.str(), "", 0);

    //关闭文件夹
    closedir(dir);

    return;
}

void my_clr(string cmd[], int argnum){
    if (argnum > 0){
        printmessage("", "Error! Too much parameters.\n", 1);
        return;
    }
    system("clear");
    state = 0;
    return;
}

void my_echo(string cmd[], int argnum){
    //没有足够的参数，报错退出
    if(argnum == 0){
        printmessage("", "Error! Need arguments.\n", 1);
        return;
    }

    stringstream ss;
    //打印主循环
    for(int i = 0; i < argnum; i++){
        string cur_word = cmd[i];
        size_t pos = cur_word.find("$");
        //在参数中找到了$字符
        if(pos == 0){
            if(cur_word[1] == '#'){
                //打印参数个数
                ss << argc - 1 << " ";
            }else if(isdigit(cur_word[1])){
                //打印参数，先把参数转化成数字
                cur_word.erase(pos, 1);
                int imm = stoi(cur_word);
                ss << argv[imm] << " ";
            }else{
                //打印环境变量
                cur_word.erase(pos, 1);
                string environ = getenv(cur_word.c_str());
                ss << environ << " ";
            }
        }else{
        //原样输出
            ss << cur_word << " ";
        }
    }
    ss << endl;
    printmessage(ss.str(), "", 0);
    return;
}

void my_pwd(string cmd[], int argnum){
    if(argnum > 0){
        printmessage("", "Error! No parameter.\n", 1);
        return;
    }
    string op = pwd + "\n";
    printmessage(op, "", 0);
    return;
}

void my_exit(string cmd[], int argnum){
    if(argnum > 0){
        printmessage("", "Error! No parameter.\n", 1);
        return;
    }
    printmessage("", "", 0);
    exit(0);
    return;
}

void my_time(string cmd[], int argnum){
    if(argnum > 0){
        printmessage("", "Error! No parameter.\n", 1);
        return;
    }

    time_t timep;   //时间结构体变量
    string wday[7] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    struct tm *p;
    time(&timep);   //获取当前时间

    stringstream ss;
    //ss << asctime(gmtime(&timep));
    
    p = localtime(&timep);
    ss << 1900 + p->tm_year << "年" << 1 + p->tm_mon << "月" << p->tm_mday << "日";
    ss << " " << wday[p->tm_wday] << " " << p->tm_hour << ":" << p-> tm_min << ":" << p->tm_sec << endl;

    printmessage(ss.str(), "", 0);
    return;
}

void my_set(string cmd[], int argnum){
    stringstream ss;
    if(argnum == 0){
        //若无参数直接输出所有的环境变量
        extern char **environ;
        for(int i = 0; environ[i] != NULL; i++){
            ss << environ[i] << " ";
        }
        ss << endl;
        printmessage(ss.str(), "", 0);
    }else{
        printmessage("", "Error! No parameter.\n", 1);
        return;
    }
    return;
}

void my_help(string cmd[], int argnum){
    stringstream ss;
    stringstream sss;

    ifstream helpfile("help"); // 打开文件，将文件名替换为你要读取的文件名

    if (!helpfile.is_open()) {
        ss << "Failed to open the file." << endl;
        printmessage("", ss.str(), 1);
        return;
    }

    string line;
    while (getline(helpfile, line)) {
        ss << line << endl; // 逐行输出文件内容
    }

    helpfile.close(); // 关闭文件

    if(argnum == 0){
        printmessage(ss.str(), "", 0);
    }else if(argnum == 1){
        //若为一参数，则进行查找
        string ins = cmd[0];
        while(getline(ss, line)){
            if(line.find(ins) != string::npos){
                sss << line << endl;
                for(int i = 0; i < 2; i++){
                    getline(ss, line);
                    sss << line << endl;
                }
                break;
            }
        }
        printmessage(sss.str(), "", 0);
    }else{
        //参数过多
        printmessage("", "Error! Too much parameters.\n", 1);
        return;
    }
    return;
}

void my_umask(string cmd[], int argnum){
    if(argnum == 0){
        //没有参数显示umask的值
        mode_t curmode = umask(0);
        umask(curmode);
        string printmode;
        printmode = to_string((curmode >> 9) & 7) + 
                    to_string((curmode >> 6) & 7) + 
                    to_string((curmode >> 3) & 7) + 
                    to_string(curmode & 7) + '\n';
        printmessage(printmode, "", 0);
    }else if(argnum == 1){
        //有一个参数则设置umask的值
        //如果长度大于4，则报错
        if(cmd[0].length() > 4){
            printmessage("", "Error! No more than four numbers.\n", 1);
            return;
        }
        //补齐位数到四位
        while(cmd[0].length() < 4) cmd[0] = '0' + cmd[0];
        if(cmd[0][0] >= '0' && cmd[0][0] <= '7' && 
           cmd[0][1] >= '0' && cmd[0][1] <= '7' && 
           cmd[0][2] >= '0' && cmd[0][2] <= '7' && 
           cmd[0][3] >= '0' && cmd[0][3] <= '7' ){
            int umasknum = 0;
            umasknum = ((cmd[0][0] - '0') << 9) + ((cmd[0][1] - '0') << 6) + 
                       ((cmd[0][2] - '0') << 3) + (cmd[0][3] - '0');
            umask(umasknum);
            printmessage("", "", 0);
            return;
        }
        //如果cmd有字符不是数字
        printmessage("", "Error! Unknown number.\n", 1);
    }else{
        //参数多于两个
        printmessage("", "Error! Too much parameters.\n", 1);
    }
    return;
}

void my_exec(string cmd[], int argnum){
    if(argnum == 0){
        printmessage("", "Error! Need parameter.\n", 1);
        return;
    }
    char *cmdarg[argnum + 1];
    //传递参数
    for(int i = 0; i < argnum; i++){
        cmdarg[i] = (char *)cmd[i].c_str();
    }
    cmdarg[argnum] = NULL;
    printmessage("", "", 0);
    execvp(cmd[0].c_str(), cmdarg);
    printmessage("", "Error! Cannot open the program.", 1);
    return;
}