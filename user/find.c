#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char* fmtname(char *path)
{
    static char buf[DIRSIZ+1];
    char *p;

    // Find first character after last slash.
    for(p = path + strlen(path); p >= path && *p != '/'; p--);
    p++;

    // Return blank-padded name.
    if(strlen(p) >= DIRSIZ)
    return p;
    memmove(buf, p, strlen(p));
    // memset(buf + strlen(p), ' ', DIRSIZ - strlen(p));  //这种写法是后面变成了空格，只是输出形式看起来没东西，但在strcmp时会有问题
    buf[strlen(p)] = '\0';   //手动赋上字符串结束标志，才能后续正确比较
    return buf;
}


void find(char *path, char *name){
    struct dirent de;
    struct stat st;
    char buf[512], *p;
    int fd;
    if((fd = open(path, 0)) < 0){
        fprintf(2, "ls: cannot open %s\n", path);
        return;
    }
    if(fstat(fd, &st) < 0){
        fprintf(2, "ls: cannot stat %s\n", path);
        close(fd);
        return;
    }

    // printf("path = %s, name = %s\n", path, name);

    switch(st.type){
        case T_FILE:   //当前目录下的同名文件
            // printf("T_FILE\n");
            // printf("this is a file ==> path = %s, target_name = %s, fmtname = %s\n", path, name, fmtname(path));
            if(strcmp(fmtname(path), name) == 0) printf("%s\n", path);   
            break;

        case T_DIR:
            // printf("T_DIR\n");
            if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
                printf("ls: path too long\n");
                break;
            }
            strcpy(buf, path);
            p = buf + strlen(buf);
            *p++ = '/';
            while(read(fd, &de, sizeof(de)) == sizeof(de)){  //当前目录下的所有文件和子目录
                // printf("de.name = %s\n", de.name);
                if(strcmp(de.name, ".") == 0 || strcmp(de.name, "..") == 0) {
                    // printf("find!\n");
                    continue; //这两个不能递归
                }
                if(de.inum == 0) continue;
                memmove(p, de.name, DIRSIZ);  //--oldpath/de.name
                p[DIRSIZ] = 0;
                if(stat(buf, &st) < 0){
                    printf("ls: cannot stat %s\n", buf);
                    continue;
                }
                find(buf, name);
            }
            break;
        }
    close(fd);
}

int main(int argc, char *argv[]){
    find(argv[1], argv[2]);

    exit(0);
}