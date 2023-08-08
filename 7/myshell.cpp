//myshell.cpp
//朱镐哲 3210103283

//*******************头文件*******************
#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <fstream>
#include <vector>
#include <signal.h>
#include <dirent.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

using namespace std;

//*******************全局变量*******************
//环境变量相关
int shellpid;           //当前进程号
string hostname;        //存储主机名
string username;        //存储用户名
string homepath;        //存储家目录路径
string pwd;             //存储当前路径
string helppath;        //存储help文件路径
string myshellpath;     //存储myshell路径
//参数相关
int argc;               //存储调用程序时的参数个数
string argv[1024];      //存储调用程序时的参数个数
//执行状态
int state;              //0表示执行成功，1表示执行出错
//输出相关
string output;          //输出到out端的字符流
string operror;         //输出到err端的字符流
//重定向相关
int InputAtterminal;    //表示输入在终端
int OutputAtterminal;   //表示输出在终端
string Infilepath;      //输入重定向文件
string Outfilepath;     //输出重定向文件
string Errfilepath;     //错误重定向文件
int ReIn;               //是否有输入重定向
int ReOut;              //是否有输出重定向
int ReErr;              //是否有错误重定向
//输入输出缓冲
char buf[1024];
//终端文件标识符
int terinput;           //终端输入的文件标识符
int teroutput;          //终端输出的文件标识符
//后台进程
struct task{
    int pid;                //存储当前job的进程号
    int state;              //状态：0-done,1-suspand,2-running
    vector<string> cmd;     //存储当前的指令内容
};
vector<struct task> jobs;   //存储job
//子进程
pid_t sonpid;               //存储子进程的进程号
//当前前台正在执行的指令
vector<string> curcmd;

//*******************函数声明*******************
void initshell(int Argc, char *Argv[]);         //启动myshell时执行此函数，进行一些初始化
void back(string cmd[], int argnum);        //判断是否有&，即判断是否后台执行
void pipeanalyze(string cmd[], int argnum);     //判断是否有管道
void analyze(string cmd[], int argnum);     //解析单条命令的主要函数，用于实现重定向和正常的指令分析
void printmessage(string mes1, string mes2, int cur_state);     //用于设置消息输出和运行状态
string explainpara(string input);       //用于解析一些变量例如$0,$HOME,$#等，返回变量值
void signal_handler(int signal);        //重新设置中断处理函数

//如下函数为具体指令的实现
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
void my_test(string cmd[], int argnum);
void my_jobs(string cmd[], int argnum);
void my_fg(string cmd[], int argnum);
void my_bg(string cmd[], int argnum);
//调用外部指令
void my_outer(string cmd[], int argnum);

//******************主函数*******************
int main(int Argc, char *Argv[]){
    //进行初始化
    initshell(Argc, Argv);

    int flag = 1;
    while(flag){
        //1.每次打印提示符号之前，先判断是否有子进程结束
        for(int i = 0; i < jobs.size(); i++){
            if(waitpid(jobs[i].pid, NULL, WNOHANG) == jobs[i].pid && jobs[i].state != 0){
                //遍历jobs数组中的所有元素，如果从其他状态变成done，则将其信息删除
                //如果输入在终端，那么进行打印
                if(InputAtterminal){
                    stringstream ss;
                    ss << "[" << i + 1 << "]" << "\t" << jobs[i].pid << "\t";
                    ss << "done" << "\t";
                    for(vector<string>::iterator j = jobs[i].cmd.begin(); j < jobs[i].cmd.end(); j++){
                        ss << *j << " ";
                    }
                    ss << endl;
                    string out = ss.str();
                    write(teroutput, out.c_str(), out.length());
                }
                //从jobs中删除此元素
                jobs.erase(jobs.begin() + i);
            }
        }

        //2.如果输入在终端，那么打印提示符
        if(InputAtterminal){
            string cur_path = pwd;
            //如果路径中包含主目录，则替换成～
            size_t pos = pwd.find(homepath);
            if(pos != string::npos){
                cur_path.erase(pos, homepath.length());
                cur_path = "~" + cur_path;
            }
            //打印提示
            snprintf(buf, sizeof(buf), "\033[36m%s@%s: \033[33m%s$ \033[?25h", username.c_str(), hostname.c_str(), cur_path.c_str());
            string print = buf;
            write(teroutput, print.c_str(), print.length());
        }
        
        //3.读入并存储命令
        char command[1024];
        int i = 0;
        //一个一个字符读入输入
        while(1){
            ssize_t ret = read(STDIN_FILENO, command + i, 1);
            if(ret <= 0){
                flag = 0;
                break;
            }else if(command[i] == '\n'){
                //把换行符换成零字符并退出循环
                command[i] = '\0';
                break;
            }
            i++;
        }
        
        //4.分割指令
        istringstream input(command);
        vector<string> cmd;
        string word;
        while(input >> word) cmd.push_back(word); //把指令按空格分成若干字符串

        //5.解析并命令
        state = 0;  //指令的执行状态
        //执行命令，这个函数中事实上集成了三个函数
        //1.back()：判断是否需要后台执行
        //2.pipeanalyze()：判断是否有管道
        //3.analyze()：实现重定向和普通的单条指令执行
        back(cmd.data(), cmd.size());
    }
    return 0;
}

//******************函数定义*******************
//启动myshell时执行此函数，进行一些初始化
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

    helppath = pwd + "/help"; //获得帮助路径

    //参数复制到全局变量
    argc = Argc;
    for(int i = 0; i < Argc; i++){
        argv[i] = Argv[i];
    }

    //linux环境下获取myshell所在路径，并设置环境变量
    char buffer[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
    if (len != -1) {
        buffer[len] = '\0';
        myshellpath = buffer;
        setenv("SHELL", buffer, 1);
    } else {
        perror("Error while getting program path");
    }

    //表示输入在终端中
    InputAtterminal = 1;

    //设置终端输入输出的位置
    terinput = open("/dev/tty", O_RDONLY);
    teroutput = open("/dev/tty", O_WRONLY);

    //如果只有一个参数，那就是正常进入
    //若两个或以上就要进行询问
    if(argc >= 2){
        //读入用户的选择
        cout << "Is the '" << argv[1] << "' an input file?[Y/n] ";
        string ans;
        cin >> ans;
        while(!(ans == "Y" || ans == "n" || ans == "y")){
            cout << "Please type in your answar again: ";
            cin >> ans;
        }

        //根据用户的选择进行相应的重定向
        if(ans == "Y" || ans == "y"){
            int fd = open(argv[1].c_str(), O_RDONLY);
            if (fd < 0) {
                //若无法打开文件 则退出
                perror("Error");
                exit(0);
            }else{
                //若能够打开文件，输入重定向为该文件
                // 复制文件描述符 fd 到标准输入（文件描述符 0）
                if (dup2(fd, STDIN_FILENO) < 0) {
                    perror("Error");
                    exit(0);
                }
            }
            close(fd); // 关闭原来的文件描述符
            InputAtterminal = 0;
        }
    }

    //表示目前没有子进程
    sonpid = -1;

    //绑定信号处理函数
    signal(SIGTSTP, signal_handler);//挂起

    return;
}

//判断是否有&，即判断是否后台执行
void back(string cmd[], int argnum){
    //设置当前指令的命令
    curcmd.clear();
    for(int i = 0; i < argnum; i++) curcmd.push_back(cmd[i]);

    //检查最后一个字符串，看是否需要后台调用
    if(argnum > 0 && cmd[argnum - 1] == "&"){
        //进程到后台执行
        pid_t son = fork();
        if(son < 0){
            printmessage("", "Error! Fork failed", 1);
            return;
        }
        if(son == 0){
            //子进程进入后台运行
            setpgid(0, 0);
            analyze(cmd, argnum - 1);
            exit(0);
        }
        if(son > 0){
            //父进程
            //将子进程的信息写入curjob数组中
            struct task curjob;
            curjob.pid = son;
            curjob.state = 2;
            for(int i = 0; i < argnum - 1; i++){
                curjob.cmd.push_back(cmd[i]);
            }
            string print("hello");
            jobs.push_back(curjob);
            //如果输入在终端，则输出信息到终端
            if(InputAtterminal){
                stringstream ss;
                ss << "[" << jobs.size() << "]" << "\t" << curjob.pid << endl;
                string print = ss.str();
                write(teroutput,print.c_str(), print.length());
            }
        }
    }else{
        //否则正常执行指令
        pipeanalyze(cmd, argnum);
    }
    return;
}

//判断是否有管道
void pipeanalyze(string cmd[], int argnum){
    pid_t curpid;

    //先扫描一遍，记录所有的管道的位置
    int pipepos[argnum];
    int pipenum = 0;
    for(int i = 0; i < argnum; i++){
        if(cmd[i] == "|"){
            pipepos[pipenum++] = i;
        }
    }

    //如果没有，则直接调用解析单条指令的函数
    if(pipenum == 0){
        analyze(cmd, argnum);
        return;
    }

    //如果有管道
    sonpid = fork();
    if(sonpid){
        //父进程等待子进程完成
        while (sonpid != -1 && !waitpid(sonpid, NULL, WNOHANG)); 
        sonpid = -1;
    }else{
        //子进程
        int oriin = dup(STDIN_FILENO);
        int oriout = dup(STDOUT_FILENO);
        char buffer[1024];
        int pipe1[2];
        int pipe2[2];

        //分一个管道和多个管道两种情况
        if(pipenum == 1){
            //若只有一个管道
            pipe(pipe1);
            pid_t cursonpid[pipenum + 1];

            for(int i = 0; i < pipenum + 1; i++){
                cursonpid[i] = fork();
                if(cursonpid[i] == 0){
                    //子进程
                    if(i == 0){
                        //第一条指令输入到stdin
                        close(pipe1[0]);    //关闭pipe1读取端
                        //dup2(oriin, STDIN_FILENO);
                        dup2(pipe1[1], STDOUT_FILENO);  //pipe1写入端关联标准输出
                        close(pipe1[1]);    //关闭pipe1写入段

                        analyze(cmd, pipepos[0]);   //调用单指令分析函数
                        exit(0);
                    }else if(i == pipenum){
                        //最后一条指令输出到stdout
                        close(pipe1[1]);    //关闭pipe1写入端
                        dup2(pipe1[0], STDIN_FILENO);   //pipe1读取端关联标准输入
                        //dup2(oriout, STDOUT_FILENO);
                        close(pipe1[0]);    //关闭pipe1读取段

                        analyze(cmd + pipepos[0] + 1, argnum - pipepos[0] - 1);     //调用单指令分析函数
                        exit(0);
                    }
                }
            }

            //父进程一定要关闭管道，不然永远子进程永远无法结束！
            close(pipe1[0]); 
            close(pipe1[1]); 

            //等待所有子进程结束
            for(int i = 0; i < pipenum + 1; i++){
                waitpid(cursonpid[i], NULL, 0);
            }

            exit(0);
        }else if(pipenum >= 2){
            //有两个以上的管道
            pipe(pipe1);
            pipe(pipe2);
            pid_t cursonpid[pipenum + 1];

            for(int i = 0; i < pipenum + 1; i++){
                cursonpid[i] = fork();
                if(cursonpid[i] == 0){
                    //子进程
                    if(i == 0){
                        //第一条指令输入到stdin
                        close(pipe1[0]);    //关闭pipe1读取端
                        close(pipe2[0]);    //关闭pipe2读取端
                        close(pipe2[1]);    //关闭pipe2写入端

                        dup2(pipe1[1], STDOUT_FILENO);  //pipe1写入端关联标准输出
                        close(pipe1[1]);    //关闭pipe1写入端

                        analyze(cmd, pipepos[i]);   //调用单指令分析函数
                        exit(0);
                    }else if(i == pipenum){
                        //最后一条指令输出到stdout
                        close(pipe1[1]);    //关闭pipe1写入端
                        close(pipe1[0]);    //关闭pipe1读取端
                        close(pipe2[1]);    //关闭pipe2写入端
                        
                        dup2(pipe2[0], STDIN_FILENO);   //pipe2写入端关联标准输入
                        close(pipe2[0]);    //关闭pipe2读取端

                        analyze(cmd + pipepos[pipenum - 1] + 1, argnum - pipepos[pipenum - 1] - 1);
                        exit(0);
                    }else{
                        //中间的指令，输入来自上一个指令的输出，同时输出到下一个指令的输入
                        close(pipe1[1]);    //关闭pipe1写入端
                        close(pipe2[0]);    //关闭pipe2读取端

                        dup2(pipe1[0], STDIN_FILENO);   //从pipe1读入数据
                        dup2(pipe2[1], STDOUT_FILENO);  //从pipe2输出数据
                        dup2(pipe2[1], pipe1[1]);       //把pipe2的输出关联到pipe1的输出端

                        close(pipe1[0]);    //关闭pipe1读取端
                        close(pipe2[1]);    //关闭pipe2写入端

                        analyze(cmd + pipepos[i - 1] + 1, pipepos[i] - pipepos[i - 1] - 1);
                        exit(0);
                    }
                }
            }

            //父进程一定要关闭管道，不然永远子进程永远无法结束！
            close(pipe1[0]); 
            close(pipe1[1]); 
            close(pipe2[0]); 
            close(pipe2[1]); 

            //等待所有子进程结束
            for(int i = 0; i < pipenum + 1; i++){
                waitpid(cursonpid[i], NULL, 0);
            }

            exit(0);
        }

        //恢复原来的标准输入输出
        dup2(oriin,STDIN_FILENO);
        dup2(oriout, STDOUT_FILENO);
        _exit(0);
    }

    return;
}

//解析单条命令的主要函数，用于实现重定向和正常的指令分析
void analyze(string cmd[], int argnum){
    // // DEBUG: 打印当前运行的指令
    // stringstream ssss;
    // for(int i = 0; i < argnum; i++){
    //     ssss << cmd[i] << " ";
    // }
    // ssss << 1 << endl;
    // string sssss = ssss.str();
    // write(teroutput, sssss.c_str(), sssss.length());

    //保存开始的输入和输出流
    int oriin = dup(STDIN_FILENO);
    int oriout = dup(STDOUT_FILENO);
    int orierr = dup(STDERR_FILENO);
    //先把重定向标志位置0
    ReIn = 0;
    ReOut = 0;
    ReErr = 0;
    //把路径重置为空字符串
    Infilepath = "";
    Outfilepath = "";
    Errfilepath = "";
    //遍历参数寻找重定向符号
    for(int i = 0; i < argnum; i++){
        //输入重定向
        if(cmd[i] == "<" || cmd[i] == "0<"){
            //获取参数路径，如果有多个，则报错
            ReIn = 1;   //设置标志位
            //如果Infilepath已经有数据，说明已经被设置过，说明有多个同类型重定向文件，报错
            if(Infilepath.length() != 0){
                printmessage("", "Error! Too much file for input.\n", 1);
                break;
            }
            Infilepath = cmd[i+1];
            //打开路径对应的文件，如果打不开则报错
            //以只读模式打开
            int Infd = open(Infilepath.c_str(), O_RDONLY);
            if(Infd < 0){
                printmessage("", "Error! No such input file.\n", 1);
                break;
            }
            //把输入流重定向为对应文件
            if(dup2(Infd, STDIN_FILENO) < 0){
                printmessage("", "Error! Infd -> STDIN_FILENO dup2 failed.\n", 1);
                break;
            }
            //关闭路径对应的文件
            close(Infd);
            //调整参数个数
            if(argnum > i) argnum = i;
        }

        //输出重定向
        if(cmd[i] == ">" || cmd[i] == "1>"){
            //获取参数路径，如果有多个，则报错
            ReOut = 1;  //设置标志位
            //如果Outfilepath已经有数据，说明已经被设置过，说明有多个同类型重定向文件，报错
            if(Outfilepath.length() != 0){
                printmessage("", "Error! Too much file for output.\n", 1);
                break;
            }
            Outfilepath = cmd[i+1];
            //打开路径对应的文件，如果打不开则报错
            //以读写模式打开，若不存在则创建文件，若存在则截断文件
            int Outfd = open(Outfilepath.c_str(), O_RDWR|O_CREAT|O_TRUNC, 0666);
            if(Outfd < 0){
                printmessage("", "Error! No such output file.\n", 1);
                break;
            }
            //把输入流重定向为对应文件
            if(dup2(Outfd, STDOUT_FILENO) < 0){
                printmessage("", "Error! Outfd -> STDOUT_FILENO dup2 failed.\n", 1);
                break;
            }
            //关闭路径对应的文件
            close(Outfd);
            //调整参数个数
            if(argnum > i) argnum = i;
        }

        if(cmd[i] == "2>"){
            //获取参数路径，如果有多个，则报错
            ReErr = 1;  //设置标志位
            //如果Errfilepath已经有数据，说明已经被设置过，说明有多个同类型重定向文件，报错
            if(Errfilepath.length() != 0){
                printmessage("", "Error! Too much file for error output.\n", 1);
                break;
            }
            Errfilepath = cmd[i+1];
            //打开路径对应的文件，如果打不开则报错
            //以读写模式打开，若不存在则创建文件，若存在则截断文件
            int Errfd = open(Errfilepath.c_str(), O_RDWR|O_CREAT|O_TRUNC, 0666);
            if(Errfd < 0){
                printmessage("", "Error! No such error output file.\n", 1);
                break;
            }
            //把输入流重定向为对应文件
            if(dup2(Errfd, STDERR_FILENO) < 0){
                printmessage("", "Error! Errfd -> STDERR_FILENO dup2 failed.\n", 1);
                break;
            }
            //关闭路径对应的文件
            close(Errfd);
            //调整参数个数
            if(argnum > i) argnum = i;
        }
        //输出重定向，追加
        if(cmd[i] == ">>" || cmd[i] == "1>>"){
            //获取参数路径，如果有多个，则报错
            ReOut = 1;  //设置标志位
            //如果Outfilepath已经有数据，说明已经被设置过，说明有多个同类型重定向文件，报错
            if(Outfilepath.length() != 0){
                printmessage("", "Error! Too much file for output.\n", 1);
                break;
            }
            Outfilepath = cmd[i+1];
            //打开路径对应的文件，如果打不开则报错
            //以读写模式打开，若不存在则创建文件，若存在则追加
            int Outfd = open(Outfilepath.c_str(), O_RDWR|O_CREAT|O_APPEND, 0666);
            if(Outfd < 0){
                printmessage("", "Error! No such output file.\n", 1);
                break;
            }
            //把输入流重定向为对应文件
            if(dup2(Outfd, STDOUT_FILENO) < 0){
                printmessage("", "Error! Outfd -> STDOUT_FILENO dup2 failed.\n", 1);
                break;
            }
            //关闭路径对应的文件
            close(Outfd);
            //调整参数个数
            if(argnum > i) argnum = i;
        }

        if(cmd[i] == "2>"){
            //获取参数路径，如果有多个，则报错
            ReErr = 1;  //设置标志位
            //如果Errfilepath已经有数据，说明已经被设置过，说明有多个同类型重定向文件，报错
            if(Errfilepath.length() != 0){
                printmessage("", "Error! Too much file for error output.\n", 1);
                break;
            }
            Errfilepath = cmd[i+1];
            //打开路径对应的文件，如果打不开则报错
            //以读写模式打开，若不存在则创建文件，若存在则追加
            int Errfd = open(Errfilepath.c_str(), O_RDWR|O_CREAT|O_APPEND, 0666);
            if(Errfd < 0){
                printmessage("", "Error! No such error output file.\n", 1);
                break;
            }
            //把输入流重定向为对应文件
            if(dup2(Errfd, STDERR_FILENO) < 0){
                printmessage("", "Error! Errfd -> STDERR_FILENO dup2 failed.\n", 1);
                break;
            }
            //关闭路径对应的文件
            close(Errfd);
            //调整参数个数
            if(argnum > i) argnum = i;
        }
    }

    //如果之前的重定向没有报错，那么正常进行解析
    if(state == 0){
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
        }else if(cmd[0] == "test"){
            my_test(cmd + 1, argnum - 1);
        }else if(cmd[0] == "jobs"){
            my_jobs(cmd + 1, argnum - 1);
        }else if(cmd[0] == "fg"){
            my_fg(cmd + 1, argnum - 1);
        }else if(cmd[0] == "bg"){
            my_bg(cmd + 1, argnum - 1);
        }else{
            //如果不是上述内部指令，则解释为外部指令
            my_outer(cmd, argnum);
        }
    }
    
    //保恢复开始的输入输出和错误流
    dup2(oriin, STDIN_FILENO);
    dup2(oriout, STDOUT_FILENO);
    dup2(orierr, STDERR_FILENO);
    //关闭刚开始的三个文件
    close(oriin);
    close(oriout);
    close(orierr);
    return;
}

//用于设置消息输出和运行状态
void printmessage(string mes1, string mes2, int cur_state){
    //设置消息和状态
    output = mes1;
    operror = mes2;
    state = cur_state;
    //这里不能使用printf，而是需要使用write，因为不一定输出到终端
    //printf("%s%s", output.c_str(), operror.c_str());
    write(STDOUT_FILENO, output.c_str(), output.length());
    write(STDERR_FILENO, operror.c_str(), operror.length());
    return;
}

//用于解析一些变量例如$0,$HOME,$#等，返回变量值
string explainpara(string input){
    string output;

    if(input == "$#"){
        output = to_string(argc);
    }else if(input[0] == '$'){ 
        //去除第一个字符'$'
        input.erase(0, 1);
        int index;
        try{
            //若转换成字符没有异常，则为参数引用
            index = stoi(input);
            output = argv[index];
        }catch (const invalid_argument& e) {
            //其他的看成环境变量
            output = getenv(input.c_str());
            if(output.c_str() == NULL) return NULL;
        }catch (const out_of_range& e) {
            //其他的看成环境变量
            output = getenv(input.c_str());
            if(output.c_str() == NULL) return NULL;
        }
    }else{
        output = input;
    }

    return output;
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

    ifstream helpfile(helppath); // 打开文件

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
                getline(ss, line);
                while(line != ""){
                    sss << line << endl;
                    getline(ss, line);
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
    printmessage("", "Error! Cannot open the program.\n", 1);
    return;
}

void my_test(string cmd[], int argnum){
    if(argnum == 0){
        printmessage("", "Error! Need parameter.\n", 1);
        return;
    }else if(argnum == 2){
        string curstr = explainpara(cmd[1]);
        //文件测试
        if(cmd[0] == "-e"){
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == -1) printmessage("false\n", "", 0);
            else printmessage("true\n", "", 0);
        }else if(cmd[0] == "-r"){
            //如果文件存在且可读则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && access(curstr.c_str(), R_OK) == 0) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-w"){
            //如果文件存在且可写则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && access(curstr.c_str(), W_OK) == 0) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-x"){
            //如果文件存在且可执行则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && access(curstr.c_str(), X_OK) == 0) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-s"){
            //如果文件存在且至少有一个字符则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && buf.st_size) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-d"){
            //如果文件存在且为目录则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && S_ISDIR(buf.st_mode) ) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-f"){
            //如果文件存在且为普通文件则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && S_ISREG(buf.st_mode) ) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-c"){
            //如果文件存在且为字符型特殊文件则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && S_ISCHR(buf.st_mode) ) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-b"){
            //如果文件存在且为块特殊文件则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && S_ISBLK(buf.st_mode) ) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-p"){
            //如果文件存在且为命名管道则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && S_ISFIFO(buf.st_mode) ) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-S"){
            //如果文件存在且为套接字则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && S_ISSOCK(buf.st_mode) ) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-g"){
            //如果文件存在且设置了gid则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && (buf.st_mode & S_ISGID)) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-u"){
            //如果文件存在且设置了uid则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && (buf.st_mode & S_ISUID)) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-G"){
            //如果文件存在且且归该组所有则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && (buf.st_gid == getgid())) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-k"){
            //如果文件存在且设置了粘着位则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && (buf.st_mode & S_ISVTX)) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-O"){
            //如果文件存在且且归该用户所有则为真
            //获取文件信息
            struct stat buf;
            int ret = lstat(curstr.c_str(), &buf);
            //判断
            if(ret == 0 && (buf.st_uid == getuid())) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-t"){
            //如果 fd 是一个与终端相连的打开的文件描述符则为真
            int fd;
            try{
                fd = stoi(curstr);
            }catch (const invalid_argument& e) {
                stringstream sss;
                sss << "Invalid argument: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }catch (const out_of_range& e) {
                stringstream sss;
                sss << "Out of range: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }
            //判断
            if(isatty(fd)) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }
        //字符串测试
        else if(cmd[0] == "-z"){
            //字符串长度为零则为真
            if(curstr.length() == 0) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(cmd[0] == "-n"){
            //字符串长度不为零则为真
            if(curstr.length()) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }
        //未定义
        else{
            printmessage("", "Error! Command not found.\n", 1);
        }
    }else if(argnum == 3){
        string str1 = explainpara(cmd[0]);
        string symbol = cmd[1];
        string str2 = explainpara(cmd[2]);

        //字符串测试
        if(symbol == "="){
            if(str1 == str2) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(symbol == "!="){
            if(str1 != str2) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }
        //文件测试
        else if(symbol == "-ef"){
            //文件读入
            struct stat buf1;
            struct stat buf2;
            int ret1 = lstat(str1.c_str(), &buf1);
            int ret2 = lstat(str2.c_str(), &buf2);
            //若文件无法读入则直接报错退出
            if(!(ret1 == 0 && ret2 == 0)){
                printmessage("", "Error! File not found.\n", 1);
                return;
            }
            //若两个文件都成功读入则进行比较
            if((buf1.st_dev == buf2.st_dev) && (buf1.st_ino == buf2.st_ino))printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(symbol == "-nt"){
            //文件读入
            struct stat buf1;
            struct stat buf2;
            int ret1 = lstat(str1.c_str(), &buf1);
            int ret2 = lstat(str2.c_str(), &buf2);
            //若文件无法读入则直接报错退出
            if(!(ret1 == 0 && ret2 == 0)){
                printmessage("", "Error! File not found.\n", 1);
                return;
            }
            //若两个文件都成功读入则进行比较
            if(buf1.st_mtime > buf2.st_mtime)printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(symbol == "-ot"){
            //文件读入
            struct stat buf1;
            struct stat buf2;
            int ret1 = lstat(str1.c_str(), &buf1);
            int ret2 = lstat(str2.c_str(), &buf2);
            //若文件无法读入则直接报错退出
            if(!(ret1 == 0 && ret2 == 0)){
                printmessage("", "Error! File not found.\n", 1);
                return;
            }
            //若两个文件都成功读入则进行比较
            if(buf1.st_mtime < buf2.st_mtime)printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }
        //数值测试
        else if(symbol == "-eq"){
            double num1; 
            double num2;
            try{
                num1 = stod(str1);
            }catch (const invalid_argument& e) {
                stringstream sss;
                sss << "Invalid argument: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }catch (const out_of_range& e) {
                stringstream sss;
                sss << "Out of range: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }
            try{
                num2 = stod(str2);
            }catch (const invalid_argument& e) {
                stringstream sss;
                sss << "Invalid argument: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }catch (const out_of_range& e) {
                stringstream sss;
                sss << "Out of range: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }
            if(num1 == num2) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(symbol == "-ne"){
            double num1; 
            double num2;
            try{
                num1 = stod(str1);
            }catch (const invalid_argument& e) {
                stringstream sss;
                sss << "Invalid argument: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }catch (const out_of_range& e) {
                stringstream sss;
                sss << "Out of range: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }
            try{
                num2 = stod(str2);
            }catch (const invalid_argument& e) {
                stringstream sss;
                sss << "Invalid argument: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }catch (const out_of_range& e) {
                stringstream sss;
                sss << "Out of range: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }
            if(num1 != num2) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(symbol == "-ge"){
            double num1; 
            double num2;
            try{
                num1 = stod(str1);
            }catch (const invalid_argument& e) {
                stringstream sss;
                sss << "Invalid argument: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }catch (const out_of_range& e) {
                stringstream sss;
                sss << "Out of range: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }
            try{
                num2 = stod(str2);
            }catch (const invalid_argument& e) {
                stringstream sss;
                sss << "Invalid argument: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }catch (const out_of_range& e) {
                stringstream sss;
                sss << "Out of range: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }
            if(num1 >= num2) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(symbol == "-gt"){
            double num1; 
            double num2;
            try{
                num1 = stod(str1);
            }catch (const invalid_argument& e) {
                stringstream sss;
                sss << "Invalid argument: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }catch (const out_of_range& e) {
                stringstream sss;
                sss << "Out of range: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }
            try{
                num2 = stod(str2);
            }catch (const invalid_argument& e) {
                stringstream sss;
                sss << "Invalid argument: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }catch (const out_of_range& e) {
                stringstream sss;
                sss << "Out of range: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }
            if(num1 > num2) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(symbol == "-le"){
            double num1; 
            double num2;
            try{
                num1 = stod(str1);
            }catch (const invalid_argument& e) {
                stringstream sss;
                sss << "Invalid argument: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }catch (const out_of_range& e) {
                stringstream sss;
                sss << "Out of range: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }
            try{
                num2 = stod(str2);
            }catch (const invalid_argument& e) {
                stringstream sss;
                sss << "Invalid argument: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }catch (const out_of_range& e) {
                stringstream sss;
                sss << "Out of range: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }
            if(num1 <= num2) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }else if(symbol == "-lt"){
            double num1; 
            double num2;
            try{
                num1 = stod(str1);
            }catch (const invalid_argument& e) {
                stringstream sss;
                sss << "Invalid argument: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }catch (const out_of_range& e) {
                stringstream sss;
                sss << "Out of range: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }
            try{
                num2 = stod(str2);
            }catch (const invalid_argument& e) {
                stringstream sss;
                sss << "Invalid argument: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }catch (const out_of_range& e) {
                stringstream sss;
                sss << "Out of range: " << e.what() << endl;
                printmessage("", sss.str(), 1);
                return;
            }
            if(num1 < num2) printmessage("true\n", "", 0);
            else printmessage("false\n", "", 0);
        }
        //未定义
        else{
            printmessage("", "Error! Command not found.\n", 1);
        }
    }else{
        printmessage("", "Error! Wrong parameter.\n", 1);
    }
    return;
}

void my_outer(string cmd[], int argnum){
    pid_t son = fork();
    int old_sonpid = sonpid;
    sonpid = son;
    if(son < 0){
        //子进程调用失败
        printmessage("", "Error! Fork failed.\n", 1);
        return;
    }else if(son == 0){//子进程
        //设置parent环境变量
        setenv("PARENT", myshellpath.c_str(), 1);
        //调用原来的exec函数
        my_exec(cmd, argnum);
        printmessage("", "Error! Cannot execute the command.\n", 1);
        exit(0);
    }else{//父进程
        while (sonpid != -1 && !waitpid(sonpid, NULL, WNOHANG));
        sonpid = -1;
        printmessage("",  "", 0);
    }
    sonpid = old_sonpid;
    return;
}

void my_jobs(string cmd[], int argnum){
    string out = "";
    //write(STDOUT_FILENO, out.c_str(), out.length());
    stringstream ss;
    for(vector<struct task>::iterator i = jobs.begin(); i < jobs.end(); i++){
        ss << "[" << i - jobs.begin() + 1 << "]" << "\t" << (*i).pid << "\t";
        switch((*i).state){
            case 0: ss << "done" << "\t"; break;
            case 1: ss << "suspend" << "\t"; break;
            case 2: ss << "running" << "\t"; break;
        }
        for(vector<string>::iterator j = (*i).cmd.begin(); j < (*i).cmd.end(); j++){
            ss << *j << " ";
        }
        ss << endl;
    }
    out = ss.str(); 
    write(STDOUT_FILENO, out.c_str(), out.length());
    return;
}

void my_fg(string cmd[], int argnum){
    //若参数个数错误则直接报错退出
    if(argnum != 1){
        printmessage("", "Error! Wrong parameter number.\n", 1);
        return;
    }
    //检查参数是否能够转化为数字
    double num; 
    try{
        num = stoi(cmd[0]);
    }catch (const invalid_argument& e) {
        stringstream sss;
        sss << "Invalid argument: " << e.what() << endl;
        printmessage("", sss.str(), 1);
        return;
    }catch (const out_of_range& e) {
        stringstream sss;
        sss << "Out of range: " << e.what() << endl;
        printmessage("", sss.str(), 1);
        return;
    }
    //检查参数是否在范围内
    num--;
    if(!(num >= 0 && num < jobs.size())){
        printmessage("", "Error! Out of range.\n", 1);
        return;
    }
    
    //参数检查通过，打印目标指令
    if(InputAtterminal){
        stringstream ss;
        for(vector<string>::iterator j = jobs[num].cmd.begin(); j < jobs[num].cmd.end(); j++){
            ss << *j << " ";
        }
        ss << endl;
        string o = ss.str();
        write(STDOUT_FILENO, o.c_str(), o.length());
    }

    setpgid(jobs[num].pid, getgid());

    //进行具体操作
    if(jobs[num].state == 1){
        //若进程被挂起，则向其发送SIGCONT信号并调⽤waitpid等待其结束

        stringstream ssss;
        ssss << num << endl << jobs[num].pid << endl;
        string sssss = ssss.str();
        write(teroutput, sssss.c_str(), sssss.length());

        kill(jobs[num].pid, SIGCONT);
        pid_t old_sonpid = sonpid;
        sonpid = jobs[num].pid;
        while (sonpid != -1 && !waitpid(sonpid, NULL, WNOHANG));
        sonpid = -1;
        sonpid = old_sonpid;
    }else if(jobs[num].state == 2){
        //若进程在后台运⾏，直接调⽤waitpid等待其结束
        pid_t old_sonpid = sonpid;
        sonpid = jobs[num].pid;
        while (sonpid != -1 && !waitpid(sonpid, NULL, WNOHANG)); 
        sonpid = -1;
        sonpid = old_sonpid;
    }

    //从jobs表中删除
    jobs.erase(jobs.begin() + num);

    return;
}

void my_bg(string cmd[], int argnum){
    //若参数个数错误则直接报错退出
    if(argnum != 1 && argnum != 0){
        printmessage("", "Error! Wrong parameter number.\n", 1);
        return;
    }

    //若没有参数，则打印后台运行的进程
    if(argnum == 0){
        string out = "";
        stringstream ss;
        for(vector<struct task>::iterator i = jobs.begin(); i < jobs.end(); i++){
            if((*i).state == 2){
                ss << "[" << i - jobs.begin() + 1 << "]" << "\t" << (*i).pid << "\t";
                ss << "running" << "\t";
                for(vector<string>::iterator j = (*i).cmd.begin(); j < (*i).cmd.end(); j++){
                ss << *j << " ";
            }
            ss << endl;
            }
        }
        out = ss.str(); 
        write(STDOUT_FILENO, out.c_str(), out.length());
        return;
    }

    //若有一个参数，则将这个进程转移⾄后台运⾏
    //检查参数是否能够转化为数字
    double num; 
    try{
        num = stoi(cmd[0]);
    }catch (const invalid_argument& e) {
        stringstream sss;
        sss << "Invalid argument: " << e.what() << endl;
        printmessage("", sss.str(), 1);
        return;
    }catch (const out_of_range& e) {
        stringstream sss;
        sss << "Out of range: " << e.what() << endl;
        printmessage("", sss.str(), 1);
        return;
    }
    //检查参数是否在范围内
    num--;
    if(!(num >= 0 && num < jobs.size())){
        printmessage("", "Error! Out of range.\n", 1);
        return;
    }

    if(jobs[num].state == 2){
        printmessage("", "Job is already running.\n", 1);
        return;
    }
    
    //参数检查通过，打印目标指令
    if(InputAtterminal){
        stringstream ss;
        for(vector<string>::iterator j = jobs[num].cmd.begin(); j < jobs[num].cmd.end(); j++){
            ss << *j << " ";
        }
        ss << "&" << endl;
        string o = ss.str();
        write(STDOUT_FILENO, o.c_str(), o.length());
    }

    //进行具体操作
    //恢复运行
    kill(jobs[num].pid, SIGCONT);
    //修改jobs表格
    jobs[num].state = 2;

    return;
}

//重新设置中断处理函数
void signal_handler(int signal){
    //重新设置Ctrl+Z中断，对应SIGSTOP信号
    if(signal == SIGTSTP){
        //以sonpid为组长创建后台进程组
        setpgid(sonpid, 0);
        //发送SIGSTOP信号，如果失败则报错
        if(kill(sonpid, SIGSTOP) != 0){
            stringstream ss;
            ss << "pid:" << sonpid << "suspended failed" << endl;
            string o = ss.str();
            write(STDERR_FILENO, o.c_str(), o.length());
        }
        //更新jobs数组
        struct task curjob;
        curjob.pid = sonpid;
        curjob.state = 1;
        for(int i = 0; i < curcmd.size(); i++){
            curjob.cmd.push_back(curcmd[i]);
        }
        string print("hello");
        jobs.push_back(curjob);
        //如果输入在终端，那么需要输出消息
        if(InputAtterminal){
            stringstream ss;
            ss << "[" << jobs.size() << "]" << "\t" << curjob.pid << "\t" << "suspend" << endl;
            string print = ss.str();
            write(teroutput,print.c_str(), print.length());
        }
        //把子进程设置为-1，也就表示没有子进程
        sonpid = -1;
    }
}