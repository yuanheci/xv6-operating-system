#include "user/user.h"
#include "kernel/types.h"

int primes[] = {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31};
char s[3];


char* itoa(int num,char* str,int radix)
{
    char index[]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";//索引表
    unsigned unum;//存放要转换的整数的绝对值,转换的整数可能是负数
    int i=0,j,k;//i用来指示设置字符串相应位，转换之后i其实就是字符串的长度；转换后顺序是逆序的，有正负的情况，k用来指示调整顺序的开始位置;j用来指示调整顺序时的交换。
 
    //获取要转换的整数的绝对值
    if(radix==10&&num<0)//要转换成十进制数并且是负数
    {
        unum=(unsigned)-num;//将num的绝对值赋给unum
        str[i++]='-';//在字符串最前面设置为'-'号，并且索引加1
    }
    else unum=(unsigned)num;//若是num为正，直接赋值给unum
 
    //转换部分，注意转换后是逆序的
    do
    {
        str[i++]=index[unum%(unsigned)radix];//取unum的最后一位，并设置为str对应位，指示索引加1
        unum/=radix;//unum去掉最后一位
 
    }while(unum);//直至unum为0退出循环
 
    str[i]='\0';//在字符串最后添加'\0'字符，c语言字符串以'\0'结束。
 
    //将顺序调整过来
    if(str[0]=='-') k=1;//如果是负数，符号不用调整，从符号后面开始调整
    else k=0;//不是负数，全部都要调整
 
    char temp;//临时变量，交换两个值时用到
    for(j=k;j<=(i-1)/2;j++)//头尾一一对称交换，i其实就是字符串的长度，索引最大值比长度少1
    {
        temp=str[j];//头部赋值给临时变量
        str[j]=str[i-1+k-j];//尾部赋值给头部
        str[i-1+k-j]=temp;//将临时变量的值(其实就是之前的头部值)赋给尾部
    }
 
    return str;//返回转换后的字符串
 
}

int main(){
    int fds[2];
    int mpid = getpid();
    pipe(fds);
    int pid = fork();

    char buf[3];
    if(pid == 0){
        int id = getpid() - mpid - 1;
        if(id > 10) exit(0);
        int p = primes[id];  //当前进程负责的质数
        read(fds[0], buf, 2);  //从左边接受
        //判断
        int x = atoi(buf);
        if(x % p == 0) {
            while(wait((int*)0) != -1);
            exit(0);  //不是质数，直接结束该进程
        }
        if(id == 10){   //判断结束，确定是质数
            printf("prime %d\n", p);
            while(wait((int*)0) != -1);
            exit(0);
        }
        //否则要往右边传
        write(fds[1], buf, strlen(buf));  
        int newpid = fork();
        if(newpid == 0){  //子进程的子进程
            int id = getpid() - mpid - 1;
            if(id > 10) exit(0);
            int p = primes[getpid() - mpid - 1];  //当前进程负责的质数
            read(fds[0], buf, 2);  //从左边接受
            //判断
            int x = atoi(buf);
            if(x % p == 0) {
                while(wait((int*)0) != -1);
                exit(0);  //不是质数，直接结束该进程
            }
            if(id == 10){   //判断结束，确定是质数
                printf("prime %d\n", p);
                while(wait((int*)0) != -1);
                exit(0);
            }
            //否则要往右边传
            write(fds[1], buf, strlen(buf));  
        }
    }
    else{
        for(int i = 2; i <= 35; i++){
            itoa(i, s, 10);  //int转成字符串 
            write(fds[1], s, strlen(s));
            while(wait((int*)0) != -1){  //表示还有子进程未结束

            }
        }
    }

    exit(0);
}