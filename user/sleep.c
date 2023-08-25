#include "user/user.h"
#include "kernel/types.h"

int main(int argc, char* argv[]){
    if(argc == 1) {
        printf("input args error\n");
        // char *s = "input args error!\n";
        // write(1, s, strlen(s));
    }
    else{
        int t = atoi(argv[1]);
        sleep(t);
    }
    exit(0);
}
