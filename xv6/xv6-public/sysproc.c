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
#include "file.h"

#define PAGE_SIZE 4096

int sys_fork(void)
{
	return fork();
}

int sys_exit(void)
{
	exit();
	return 0; // not reached
}

int sys_wait(void)
{
	return wait();
}

int sys_kill(void)
{
	int pid;

	if (argint(0, &pid) < 0)
		return -1;
	return kill(pid);
}

int sys_getpid(void)
{
	return myproc()->pid;
}

int sys_sbrk(void)
{
	int addr;
	int n;

	if (argint(0, &n) < 0)
		return -1;
	addr = myproc()->sz;
	if (growproc(n) < 0)
		return -1;
	return addr;
}

int sys_sleep(void)
{
	int n;
	uint ticks0;

	if (argint(0, &n) < 0)
		return -1;
	acquire(&tickslock);
	ticks0 = ticks;
	while (ticks - ticks0 < n)
	{
		if (myproc()->killed)
		{
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
int sys_uptime(void)
{
	uint xticks;

	acquire(&tickslock);
	xticks = ticks;
	release(&tickslock);
	return xticks;
}

uint sys_wmap(void)
{
	// decl args
	int tAddr;
	int length;
	int flags;
	int fd;

	argint(0, &tAddr);
	argint(1, &length);
	argint(2, &flags);
	argint(3, &fd);

	uint addr = PGROUNDUP(tAddr);
	if (length <= 0)
	{
		return -1;
	}

	// NEW: commented this line out
	// PGROUNDUP(addr);

	struct lazy *temp = myproc()->head;
	struct lazy *tail = myproc()->tail;

	cprintf("address: %d", addr);

	// myproc() use to retrieve proc struct

		if (addr + length >= KERNBASE)
		{
			return -1;
		}
		if (temp->length == 0)
		{
			temp->used = 1;
			temp->addr = addr;
			temp->length = length;
			temp->fd = (flags & MAP_ANONYMOUS) ? -1 : fd;
			temp->shared = (flags & MAP_SHARED) ? 1 : 0;
			myproc()->tail = temp;
			return addr;
		}
		uint last = 0;
		while (temp)
		{
			if (addr < temp->addr && (addr + length) < temp->addr)
			{
				struct lazy *new = (struct lazy *)kalloc();
				memset(new, 0, sizeof(struct lazy));
				new->addr = addr;
				new->length = length;
				new->fd = (flags & MAP_ANONYMOUS) ? -1 : fd;
				new->shared = (flags & MAP_SHARED) ? 1 : 0;
				new->used = 1;
				new->next = temp;
				if (temp->prev)
				{
					new->prev = temp->prev;
					temp->prev->next = new;
					temp->prev = new;
				}
				else
				{
					temp->prev = new;
				}
				if(last == 0) {
					myproc()->head = new;
				}
				return addr;
			}
			else if (addr < temp->addr && (addr + length) >= temp->addr && (flags & MAP_FIXED))
			{
				return -1;
			}
			last = temp->addr + temp->length;
			temp = temp->next;
		}
		cprintf("last: %d", last);
		if (addr < KERNBASE && last <= addr)
		{
			struct lazy *new = (struct lazy *)kalloc();
			memset(new, 0, sizeof(struct lazy));
			new->addr = addr;
			new->length = length;
			new->fd = (flags & MAP_ANONYMOUS) ? -1 : fd;
			new->shared = (flags & MAP_SHARED) ? 1 : 0;
			new->used = 1;
			tail->next = new;
			new->prev = tail;
			myproc()->tail = new;
			return addr;
		}

	temp = myproc()->head;
	if(!(MAP_FIXED & flags))
	{
		uint start = MMAPBASE;
		while (temp)
		{
			uint end = temp->addr;
			cprintf("do we enter here\n");
			if (end - start > length)
			{
				struct lazy *new = (struct lazy *)kalloc();
				memset(new, 0, sizeof(struct lazy));
				new->addr = start;
				new->length = length;
				new->fd = (flags & MAP_ANONYMOUS) ? -1 : fd;
				new->shared = (flags & MAP_SHARED) ? 1 : 0;
				new->used = 1;
				new->next = temp;
				if (temp->prev)
				{
					new->prev = temp->prev;
					temp->prev->next = new;
					temp->prev = new;
				}
				else
				{
					temp->prev = new;
				}
				if(start == 0) {
					myproc()->head = new;
				}
				return addr;
			}
			start = temp->addr + temp->length;
			temp = temp->next;
		}
		cprintf("Here?");
		if (KERNBASE - start > length)
		{
			cprintf("Enters here?\n");
			struct lazy *new = (struct lazy *)kalloc();
			memset(new, 0, sizeof(struct lazy));
			new->addr = start;
			new->length = length;
			new->fd = (flags & MAP_ANONYMOUS) ? -1 : fd;
			new->shared = (flags & MAP_SHARED) ? 1 : 0;
			new->used = 1;
			new->prev = tail;
			tail->next = new;
			myproc()->tail = new;
			return addr;
		}
	}
	return -1;
}

int sys_wunmap(void)
{
	int taddr;
	argint(0, &taddr);
	uint addr = (uint)taddr;

	// invalid addr check
	if (taddr < 1610612736 || taddr > 2147483648)
	{
		return -1;
	}
	struct lazy *temp = myproc()->head;
	while (temp)
	{
		if (temp->addr == addr)
		{
			if (temp->prev && temp->next)
			{ // UPDATES LAZY STRUCT
				temp->prev->next = temp->next;
				temp->next->prev = temp->prev;
			}
			else if (temp->prev)
			{
				temp->prev->next = temp->next;
			}
			else if (temp->next)
			{
				temp->next->prev = temp->prev;
			}
			else
			{
				struct lazy *new = (struct lazy *)kalloc();
				memset(new, 0, sizeof(struct lazy));
				myproc()->head = new;
			}
			uint *pte;
			if (temp->fd == -1 || temp->shared == 0)
			{ // UPDATES PTE'S (removes)
				for (int i = 0; i < temp->length; i += 4096)
				{
					pte = walkpgdir(myproc()->pgdir, (char *)addr + i, 0);
					kfree(P2V(PTE_ADDR(*pte)));
				}
				pte = 0;
			}
			else
			{
				struct file *f = myproc()->ofile[temp->fd];
				for (int i = 0; i < temp->length; i++)
				{
					pte = walkpgdir(myproc()->pgdir, (char *)addr + i, 0);
					f->off = addr;
					uint buf = PTE_ADDR(*pte);
					if (pte != 0)
					{
						filewrite(f, (char *)buf, 4096);
					}
					kfree(P2V(buf));
				}
			}
			// NEED TO DO: UPDATE VA->PA mapping in proc struct, and make sure to free temp (node of lazy struct), because its no longer in use. Maybe using an array is actual
			// better because then no need to free, just set to 0. Array size for lazy struct is 16, that is pre-defined.
			// kfree(temp); SHOULD NEED SOMETHING LIKE THIS

			// NEW (check)
			// NOTE: need to free address? (temp -> addr)
			// updating mapping metadata in proc struct (va -> pa mappings).
			for (int i = 0; i < 32; i++)
			{
				if (temp->addr == myproc()->va[i])
				{
					myproc()->va[i] = 0; // setting invalid va  prev: -1
					myproc()->pa[i] = -1; // setting invalid pa (removed)
					break;
				}
			}
			// free temp
			kfree((char *)temp);
			return 0;
		}
		temp = temp->next;
	}
	return -1;
}

uint sys_wremap(void)
{
	int tempoldaddr;
	uint oldaddr;
	int oldsize;
	int newsize;
	int flags;

	argint(0, &tempoldaddr);
	argint(1, &oldsize);
	argint(2, &newsize);
	argint(3, &flags);

	oldaddr = (uint)tempoldaddr;

	// NEW: error checks:
	if (newsize <= 0)
	{
		return -1;
	}
	if (!flags & MREMAP_MAYMOVE && !flags & 0)
	{
		return -1;
	}

	struct lazy *temp = myproc()->head;
	while (temp)
	{
		if (temp->addr == oldaddr)
		{ // UPDATING LAZY STRUCT
			if (temp->length != oldsize)
			{
				return -1;
			}
			uint upper = (temp->next) ? temp->next->addr : KERNBASE;
			if (oldaddr + newsize < upper)
			{
				temp->length = newsize;

				// NEW:
				// NOTE: update PTE? proc VA->PA mapping? numPages(lazy struct)? sz(proc struct)?

				return oldaddr;
			}
			else if (flags & MREMAP_MAYMOVE)
			{
				struct lazy *temp2 = myproc()->head;

				// NOTE: don't think this is correct logic using start=0
				int start = MMAPBASE;
				while (temp2)
				{
					if(temp2->addr == oldaddr) {
						temp2 = temp2->next;
						continue;
					}
					int end = temp2->addr;
					if (end - start > newsize)
					{
						temp->addr = start;
						temp->length = newsize;

						// NEW: UPDATE PTE's FOR VA->PA Mapping + UPDATE PROC VA->PA mapping
						// NOTE: any other check or error condition for the phys address obtained?
						// NOTE: free old physical memory?
						pte_t *pte = walkpgdir(myproc()->pgdir, (void *)temp->addr, 0); // update PTE
						*pte = (V2P(temp->addr) & ~0xFFF) | flags;

						for (int i = 0; i < 32; i++) // update proc VA->PA mapping
						{
							if (oldaddr == myproc()->va[i])
							{
								myproc()->va[i] = temp->addr;	  // updating va[i] with new va
								myproc()->pa[i] = PTE_ADDR(*pte); // updating pa[i] with new pa
								break;
							}

							return 0;
						}
						start = temp2->addr + temp2->length;
						temp2 = temp2->next;
					}
					if (KERNBASE - start > newsize)
					{
						temp->addr = start;
						temp->length = newsize;

						// NEW:
						// NOTE: any other check or error condition for the phys address obtained?
						// NOTE: free old physical memory?
						pte_t *pte = walkpgdir(myproc()->pgdir, (void *)temp->addr, 0); // update PTE
						*pte = (V2P(temp->addr) & ~0xFFF) | flags;

						for (int i = 0; i < 32; i++) // update proc VA->PA mapping
						{
							if (oldaddr == myproc()->va[i])
							{
								myproc()->va[i] = temp->addr;	  // updating va[i] with new va
								myproc()->pa[i] = PTE_ADDR(*pte); // updating pa[i] with new pa
								break;
							}
						}

						return 0;
					}
				}
				break;
			}
		}
		temp = temp->next;
	}
	return -1;
}

// NEW: fixed type casting
// NOTE: add return -1 for failure case?
int sys_getpgdirinfo(void)
{
	struct pgdirinfo *pdinfo;
	argptr(0, (void *)&pdinfo, sizeof(*pdinfo));

	struct proc *curproc = myproc();
	//pdinfo->n_upages = curproc->n_upages;


	//NEW: logic for updating pdinfo struct
	pde_t *pgdir = curproc->pgdir;
	pte_t *pte;
	int count = 0;

	// setting n_upages by traversing page directory and table
	for (int i = 0; i < NPDENTRIES; i++)
	{
		// if page table present
		if (pgdir[i] & PTE_P)
		{
			// get pt address
			pte_t *pgtab = (pte_t *)P2V(PTE_ADDR(pgdir[i]));

			// traverse page table entries
			for (int j = 0; j < NPTENTRIES; j++)
			{
				pte = &pgtab[j];

				// if valid inc n_upages & update va and pa mappings
				if (*pte & PTE_P && (*pte & PTE_U) && count < 32)
				{
					pdinfo -> va[count] = PGADDR(i, j, 0);
					pdinfo -> pa[count] = PTE_ADDR(*pte);
					count++;
				}
			}
		}
	}

	pdinfo -> n_upages = count;
	curproc -> n_upages = count;

	for (uint i = 0; i < pdinfo->n_upages; i++) //cuproc -> n_upages
	{
		curproc->va[i] = pdinfo -> va[i];
		curproc->pa[i] = pdinfo -> pa[i];
		// pdinfo->va[i] = curproc->va[i];
		// pdinfo->pa[i] = curproc->pa[i];
	}
	return SUCCESS;
}

// NEW (check): updated temp value each iteration, added return 0
// NOTE: add return -1 for invalid address?
int sys_getwmapinfo(void)
{
	int addr;
	struct wmapinfo *wminfo;

	argint(0, &addr);
	wminfo = (struct wmapinfo *)addr;

	wminfo -> total_mmaps = myproc() -> n_upages;
	
	int count = 0;
	struct lazy *temp = myproc()->head;
	while (temp && temp->used == 1)
	{
		count++;
		wminfo->addr[count-1] = temp->addr;
		wminfo->length[count-1] = temp->length;
		wminfo->n_loaded_pages[count-1] = temp->numPages;
		wminfo->total_mmaps++;

		temp = temp->next;
	}
	return 0;
}
