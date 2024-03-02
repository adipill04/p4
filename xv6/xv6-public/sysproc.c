#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "types.h"
#include "wmap.h"

#define PAGE_SIZE 4096

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return myproc()->pid;
}

int
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

int
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

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint
wmap(void)
{
  //decl args
  uint addr;
  int length; 
  int flags; 
  int fd;

  argint(0, &addr);
  argint(1, &length);
  argint(2, &flags);
  argint(3, &fd);

  //myproc() use to retrieve proc struct
  if(flags & MAP_ANONYMOUS){

  }
  if(flags & MAP_FIXED){

  }

  // for(int i = 0; i < (int) ceil((double)length / PAGE_SIZE); i++) {
  // 	char* mem = kalloc();
  // 	mappages(myproc()->pgdir, addr, PAGE_SIZE, V2P(mem), PTE_W | PTE_U);
  // }  
}

int 
wunmap(void)
{
	
}

uint
wremap(void)
{
	
}

int getpgdirinfo(void)
{
	
}

int getwmapinfo(void)
{
	
}
