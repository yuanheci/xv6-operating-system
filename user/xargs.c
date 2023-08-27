#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"


int main(int argc, char *argv[]){
    //从标准输入中按行读取
    // printf("argc = %d\n", argc);
    // for(int i = 1; i < argc; i++) printf("%s\n", argv[i]);

    char buf[100];
    int idx = 0;
    while(read(0, &buf[idx], 1) != 0){  //标准输入的fd是0，就是终端
        if(buf[idx] == '\n'){
            //一行读取完了，要传递给grep
            buf[idx] = '\0';  //!!关键
            int pid = fork();
            if(pid == 0){
                // char* s[] = {"grep", argv[2], buf, 0};  //argv[2]就是pattern...
                char *s[10];
                int idx = 0;
                for(int i = 1; i < argc; i++) s[idx++] = argv[i];
                s[idx++] = buf, s[idx] = 0;
                exec(argv[1], s);
                printf("exec failed!\n");
                exit(1);
            }
            else{
                wait((int*)0);
                idx = 0;
            }
        }
        else idx++;
    }
    // printf("%s", buf);

    exit(0);
}