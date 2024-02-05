#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

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
  backtrace();
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

static uint64 
get_systick(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// a0: alarm interval in ticks, 
// a1: pointer to handler
uint64 
sys_sigalarm(void)
{
  int alarmIntervalTicks;
  uint64 handlerAddr;
  struct proc *p = myproc();
  
  if(argint(0, &alarmIntervalTicks) < 0)
    return -1;
  if(argaddr(1, &handlerAddr) < 0)
    return -1;

  // alarm interval in ticks
  p->ticksToAlarm = alarmIntervalTicks;
  // Capture current tick
  p->alarmTick = get_systick();
  // Address of alarm handler in user space
  p->handlerAddr = handlerAddr;

  return 0;
}

uint64 
sys_sigreturn(void)
{
  struct proc *p = myproc();

  if (p->isInHandler == 1) {
    // Exit the handler
    p->isInHandler = 0;
    // Save the original user trapframe
    memmove(p->trapframe, p->bkptrapframe, sizeof(struct trapframe));
  }

  return 0;
  
}


