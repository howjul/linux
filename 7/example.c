#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>

int main() {
    int fd = open("ins", O_RDONLY);
    if (fd < 0) {
        perror("Failed to open input.txt");
        return 1;
    }

    // 复制文件描述符 fd 到标准输入（文件描述符 0）
    // if (dup2(fd, STDIN_FILENO) < 0) {
    //     perror("dup2 failed");
    //     close(fd);
    //     return 1;
    // }

    close(fd); // 关闭原来的文件描述符

    // 现在程序将从 input.txt 读取输入数据
    char buffer[100];
    while(1){
        ssize_t ret = read(STDIN_FILENO, buffer, 1);
        if(ret <= 0) break;
        printf("%s", buffer);
    }

    return 0;
}
