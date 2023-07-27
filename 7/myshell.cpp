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
string argv[1024];
//执行状态
int state;


void initshell(int Argc, char *Argv[]);
void analyze(string cmd[], int argnum);

void my_cd(string cmd[], int argnum);
void my_dir(string cmd[], int argnum);
void my_clr(string cmd[], int argnum);
void my_echo(string cmd[], int argnum);

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
        perror("Error! No more then one parameter.");
        state = 1;
        return;
    }

    //检查路径
    if(dir == NULL){
        perror("Error! Cannot open directory.");
        state = 1;
        return;
    }

    //打印内容
    while((ptr = readdir(dir)) != NULL){
        printf("%s ", ptr->d_name);
    }
    printf("\n");

    //关闭文件夹
    closedir(dir);

    state = 0;
    return;
}

void my_clr(string cmd[], int argnum){
    if (argnum > 0){
        perror("Error! Too much parameters.");
        state = 1;
        return;
    }
    system("clear");
    state = 0;
    return;
}

void my_echo(string cmd[], int argnum){
    //没有足够的参数，报错退出
    if(argnum == 0){
        perror("Error! Need arguments.");
        state = 1;
        return;
    }

    //打印主循环
    for(int i = 0; i < argnum; i++){
        string cur_word = cmd[i];
        size_t pos = cur_word.find("$");
        //在参数中找到了$字符
        if(pos == 0){
            if(cur_word[1] == '#'){
                //打印参数个数
                printf("%d ", argc - 1);
            }else if(isdigit(cur_word[1])){
                //打印参数，先把参数转化成数字
                cur_word.erase(pos, 1);
                int imm = stoi(cur_word);
                printf("%s ", argv[imm].c_str());
            }else{
                //打印环境变量
                cur_word.erase(pos, 1);
                string environ = getenv(cur_word.c_str());
                printf("%s ", environ.c_str());
            }
        }else{
        //原样输出
            printf("%s ", cur_word.c_str());
        }
    }
    printf("\n");
    return;
}