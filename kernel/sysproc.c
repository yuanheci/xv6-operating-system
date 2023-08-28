#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "sysinfo.h"
#include "kalloc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

//内核态syscall调用了sys_trace，用argint获取用户的输入参数，也就是trace掩码（存放在寄存器a0中）
//用来给进程设置trace掩码
uint64 sys_trace(void){
    int n;
    if(argint(0, &n) < 0)
        return -1;
    myproc()->mask = n;
    return 0;
}

uint64 sys_sysinfo(void){
    struct proc *p = myproc();
    uint64 st;
    struct sysinfo ksf;
    ksf.freemem = getfreebytes(), ksf.nproc = getprocnum();
    if(argaddr(0, &st) < 0)
        return -1;
    int res = copyout(p->pagetable, st, (char*)&ksf, sizeof ksf);
    // printf("res = %d\n", res);
    if(res < 0) return -1;
    return 0;
}
