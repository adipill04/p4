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
#define KERNBASE 536870912

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

  if(length <= 0) {
  	return -1;
  }

  if(length % 4096 != 0) {
  	length += 4096 - (length % 4096);
  }

  struct lazy* temp = myproc()->head;

  //myproc() use to retrieve proc struct
	  if(flags & MAP_FIXED){
	    if(addr + length >= KERNBASE) {
	    	return -1;
	    }
	    if(temp->length == 0) {
		  temp->used = 1;
		  temp->addr = addr;
		  temp->length = length;
		  temp->fd = (flags & MAP_ANONYMOUS) ? -1: fd;
		  temp->shared = (flags & MAP_SHARED) ? 1: 0;
		  return 0;
		}
		while(temp) {
		  if(addr < temp->addr && (addr + length) < temp->addr) {
		    struct lazy* new;
		    memset(new, 0, sizeof(struct lazy));
		    new->addr = addr;
		    new->length = length;
		    new->fd = (flags & MAP_ANONYMOUS) ? -1: fd;
		    new->shared = (flags & MAP_SHARED) ? 1: 0;
		    new->used = 1;
		    new->next = temp;
		    if(temp->prev){
		      new->prev = temp->prev;
		      temp->prev->next = new;
		      temp->prev = new;
		    } else {
		      temp->prev = new;
		    }
		    return 0;
		  } else if(addr < temp->addr && (addr + length) >= temp->addr) {
		  	return -1;
		  }
		  temp = temp->next;
		}
  } else{
    if(temp->length == 0) {
    		  temp->used = 1;
    		  temp->addr = 0;
    		  temp->length = length;
    		  temp->fd = (flags & MAP_ANONYMOUS) ? -1: fd;
    		  temp->shared = (flags & MAP_SHARED) ? 1: 0;
    		  return 0;
    		}
    int start = 0;
    while(temp) {
        int end = temp->addr;
    	if(end - start > length) {
    		struct lazy* new;
    		memset(new, 0, sizeof(struct lazy));
    		new->addr = start;
    		new->length = length;
    		new->fd = (flags & MAP_ANONYMOUS) ? -1: fd;
    		new->shared = (flags & MAP_SHARED) ? 1 : 0;
    		new->used = 1;
    		new->next = temp;
    		if(temp->prev){
    			new->prev = temp->prev;
    			temp->prev->next = new;
    			temp->prev = new;
    		} else {
    			temp->prev = new;
    		}
    		return 0;
    	}
    	start = temp->addr + temp->length;
    	temp = temp->next;
    }
    if(KERNBASE - start > length) {
    	struct lazy* new;
    	    		memset(new, 0, sizeof(struct lazy));
    	    		new->addr = start;
    	    		new->length = length;
    	    		new->fd = (flags & MAP_ANONYMOUS) ? -1: fd;
    	    		new->shared = (flags & MAP_SHARED) ? 1 : 0;
    	    		new->used = 1;
    	    		new->next = temp;
    	    		if(temp->prev){
    	    			new->prev = temp->prev;
    	    			temp->prev->next = new;
    	    			temp->prev = new;
    	    		} else {
    	    			temp->prev = new;
    	    		}
    	    		return 0;
    }
    return -1;
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
