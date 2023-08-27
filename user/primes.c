#include "user/user.h"
#include "kernel/types.h"

//利用管道实现的并发形式的埃氏筛素数

void solve(int rfd){
    int pipefd[2], fork_flag = 0, primes = 0;  //fork标志位，当前进程使用的质数
    int c;
    while(1){
        int res = read(rfd, &c, 4); //write端关闭时，当所有数据读完后，再读会返回0
        if(res == 0){  //等待子进程结束即可
            close(rfd);
            if(fork_flag){
                close(pipefd[1]);
                wait((int*)0);
            }
            exit(0);
        }

        //从left读到的第一个数就是当前进程需要使用的质数
        if(primes == 0) {
            primes = c;
            printf("prime %d\n", primes);
        }
        if(c % primes){  //说明不能过滤掉，要传给子进程去继续判断
            if(!fork_flag){
                pipe(pipefd);
                fork_flag = 1;
                int pid = fork();
                if(pid == 0){
                    close(pipefd[1]);
                    close(rfd);
                    solve(pipefd[0]);      
                }
                else{
                    close(pipefd[0]);
                }
            }
            write(pipefd[1], &c, 4);
        }
    }
}

int main(){
    int pipefd[2];
    pipe(pipefd);
    for(int i = 2; i <= 35; i++){
        write(pipefd[1], &i, 4);
    }
    close(pipefd[1]);
    solve(pipefd[0]);   //传入读端
    exit(0);
}