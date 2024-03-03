#include "param.h"
#include "types.h"
#include "defs.h"
#include "x86.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "elf.h"
#include "date.h"
#include "wmap.h"
#include "vm.h"
#include "file.h"

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
  int tAddr;
  int length; 
  int flags; 
  int fd;

  argint(0, &tAddr);
  argint(1, &length);
  argint(2, &flags);
  argint(3, &fd);

  uint addr = PGROUNDUP(tAddr);
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
		uint last;
		while(temp) {
		  if(addr < temp->addr && (addr + length) < temp->addr) {
		    struct lazy* new = (struct lazy*)kalloc();
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
		  last = temp->addr + temp->length;
		  temp = temp->next;
		}
		if(addr < KERNBASE && last < addr) {
			struct lazy* new = (struct lazy*)kalloc();
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
    		struct lazy* new = (struct lazy*)kalloc();
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
    	struct lazy* new = (struct lazy*)kalloc();
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
  }
  return -1;
}

int 
wunmap(void)
{
  int taddr;
  argint(0, &taddr);
  uint addr = (uint)taddr;  

  struct lazy* temp = myproc()->head;
  while(temp) {
  	if(temp->addr == addr) {
  	   if(temp->prev && temp->next) {
  	  	  temp->prev->next = temp->next;
  	  	  temp->next->prev = temp->prev;
  	  	} else if(temp->prev){
  	  		temp->prev->next = temp->next;
  	  	} else if(temp->next){
  	  		temp->next->prev = temp->prev;
  	  	} else {
  	  		struct lazy* new = (struct lazy*)kalloc();
  	  		memset(new, 0, sizeof(struct lazy));
  	  		myproc()->head = new;
  	  	}
  	  	pte_t *pte;
	  	if(temp->fd == -1 || temp->shared == 0) {
	  	  for(uint i = 0; i < temp->length; i += 4096) {
		  	  pte = walkpgdir(myproc()->pgdir, (char *)addr+i, 0);
		  	  kfree(P2V(PTE_ADDR(*pte)));
		  }
		  pte = 0;
	  	} else {
	  	  struct file *f = myproc()->ofile[temp->fd];
	  	  for(int i = 0; i < (temp->length / 4096); i++) {
	  	  	pte = walkpgdir(myproc()->pgdir, addr + (i * 4096), 0);
	  	    f->off = addr;
	  	  	char* buf = PTE_ADDR(*pte);
	  	  	if(pte != 0) {
	  	  		filewrite(f, buf, 4096);
	  	  	}
	  	  	kfree(P2V(buf));
	  	  }
	  	  	
  	  }
  	}
  	temp = temp->next;
  }
}

uint
wremap(void)
{
  uint oldaddr;
  int oldsize;
  int newsize;
  int flags;

  argint(0, &oldaddr);
  argint(1, &oldsize);
  argint(2, &newsize);
  argint(3, &flags);

  struct lazy*temp = myproc()->head;
  while(temp) {
  	if(temp->addr == oldaddr) {
  	  if(temp->length != oldsize) {
  	  	return -1;
  	  }
  	  uint upper = (temp->next) ? temp->next->addr : KERNBASE;
  	  if(oldaddr + newsize < upper) {
  	  	temp->length = newsize;
  	  	return 0;
  	  } else if(flags & MREMAP_MAYMOVE) {
  	    struct lazy* temp2 = myproc()->head;
  	    int start = 0;
  	    while(temp2) {
  	      int end = temp2->addr;
  	    	if(end - start > newsize) {
  	    	  temp->addr = start;
  	    	  temp->length = newsize;
  	    	  return 0;
  	    	}
  	    	start = temp2->addr + temp2->length;
  	    	temp2 = temp2->next;
  	    }
  	    if(KERNBASE - start > newsize) {
  	    	temp->addr = start;
  	    	temp->length = newsize;
  	    	return 0;
  	    }
  	  }
  	  break;
  	}
  	temp = temp->next;
  }
  return -1;
}

int getpgdirinfo(void)
{
  int addr;
  struct pgdirinfo *pdinfo;

  argint(0, &addr);
  pdinfo = (pgdirinfo *)addr;
  
    struct proc *curproc = myproc();
    pde_t *pgdir = curproc->pgdir;
    int count = 0;

    for (int i = 0; i < NPDENTRIES && count < MAX_UPAGE_INFO; i++) {
        if (pgdir[i] & PTE_P) {  // Check if the page directory entry is present
            pte_t *pgtab = (pte_t*)P2V(PTE_ADDR(pgdir[i]));
            for (int j = 0; j < NPTENTRIES; j++) {
                if (pgtab[j] & PTE_P && pgtab[j] & PTE_U) {  // Check if the page table entry is present and user-accessible
                    // Assuming pdinfo->va and pdinfo->pa are arrays to store virtual and physical addresses
                    pdinfo->va[count] = (void*)((i << PDXSHIFT) | (j << PTXSHIFT));
                    pdinfo->pa[count] = P2V(PTE_ADDR(pgtab[j]));
                    count++;
                    if (count >= MAX_UPAGE_INFO) break;  // Stop if we've collected enough info
                }
            }
        }
    }
 
}

int getwmapinfo(void)
{
  int addr;
  struct wmapinfo *wminfo;

  argint(0, &addr);
  wminfo = (struct wmapinfo*)addr;
  
  int maps = 0;
  struct lazy* temp = myproc()->head;
  while(temp) {
  	wminfo->addr[maps] = temp->addr;
  	wminfo->length[maps] = temp->length;
  	wminfo->n_loaded_pages[maps] = temp->numPages;
  	maps++;
  	wminfo->total_mmaps++;
  }
}
