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

  void insertInOrder(int arr[], int num) {
      int i;
      for (i = 0; i < 34; i++) {
          // Find the position where number should be inserted
          if (arr[i] == -1 || arr[i] > num) {
              break;
          }
      }
  
      // Shift elements to the right to make space
      for (int j = 34 - 1; j > i; j--) {
          arr[j] = arr[j - 1];
      }
  
      // Insert the number
      arr[i] = num;
  }
  
  //myproc() use to retrieve proc struct
	  if(flags & MAP_FIXED){
	  	int i = 0;
	  	while(myproc()->lazyAllocs[i] != 0 && i != 16) {
	  	  uint end1 = addr + length;
	  	  uint end2 = myproc()->lazyAllocs[i].addr + myproc()->lazyAllocs[i].length;
	      if(addr <= end2 && end1 >= myproc()->lazyAllocs[i].add) {
	  	    return -1;
	  	  }
	  	  i++;
	  	}
	  	if(i == 16) {
	  		return -1;
	  	}
	  	myproc()->lazyAllocs[i].addr = addr;
	  	myproc()->lazyAllocs[i].length = length;
	  	myproc()->lazyAllocs[i].fd = (flags & MAP_ANONYMOUS) ? -1: fd;
	  	myproc()->lazyAllocs[i].shared = (flags & MAP_SHARED) ? 1 : 0;
	  }
	  else{
	  	int address[34];
	  	for (int i = 0; i < 34; i++) {
	  	  address[i] = -1;
	  	}
	  	address[0] = 0;
	  	address[1] = 536870912;
	  	
	  	
	  	int i = 0;
	  	while(myproc()->lazyAllocs[i] != 0 && i != 16) {
	  	  uint end = myproc()->lazyAllocs[i].addr + length;
		  insertInOrder(address, myproc()->lazyAllocs[i].addr);
		  insertInOrder(address, end);
	  	  i++;
	  	}
	  	for(int j = 0; j < 17; j++) {
	  		if(address[j * 2 + 1] - address[j * 2] >= length) {
	  			myproc()->lazyAllocs[i].addr = address[j * 2] + 1;
	  			myproc()->lazyAllocs[i].length = length;
	  			myproc()->lazyAllocs[i].fd = (flags & MAP_ANONYMOUS) ? -1 : fd;
	  			myproc()->lazyAllocs[i].shared = (flags & MAP_SHARED) ? 1 : 0;
	  		}
	  	}
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
