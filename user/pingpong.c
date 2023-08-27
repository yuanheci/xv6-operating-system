#include "user/user.h"
#include "kernel/types.h"

int main(){
    int fds[2], n;
    char buf[2];
    pipe(fds);
    int pid = fork();
    if(pid == 0){
        n = read(fds[0], buf, 1);   //管道会阻塞
        close(fds[0]);
        if(n){
            printf("%d: received ping\n", getpid());
            write(fds[1], "1", 1);
            close(fds[1]);
        }
    }
    else{
        write(fds[1], "1", 1);
        close(fds[1]);
        //得等到子进程从管道中把数据读走，父进程才能继续往下执行
        //一个进程读走管道中的数据后，数据会在管道中被删除
        wait((int*)0);  //等待子进程结束
        n = read(fds[0], buf, 1);
        close(fds[0]);
        if(n > 0){
            printf("%d: received pong\n", getpid());
        }
    }

    exit(0);
}