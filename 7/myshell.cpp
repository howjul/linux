//myshell.cpp
//朱镐哲 3210103283

//头文件
#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
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
string argv[512];
//执行状态
int state;


void initshell(int Argc, char *Argv[]);
void analyze(string cmd[], int argnum);

void my_cd(string cmd[], int argnum);
void my_dir(string cmd[], int argnum);

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

    //如果有两个参数，那就是正常进入
    if(Argc == 2){

    }

    return;
}

void analyze(string cmd[], int argnum){
    if(cmd[0] == "cd"){
        my_cd(cmd + 1, argnum - 1);
    }
    return ;
}

void my_cd(string cmd[], int argnum){
    //如果参数个数不为1，直接报错退出
    if (argnum != 1){
        perror("Error! Usage: cd <path>");
        state = 1;
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
        perror("Error changing directory");
        state = 1;
        return;
    }
    state = 0;
    return;
}

void my_dir(string cmd[], int argnum){

    
    state = 0;
    return;
}