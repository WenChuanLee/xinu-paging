#include <xinu.h>

local	int newpid();
uint32  pageRepPolicy = FIFO;

#define	roundew(x)	( (x+3)& ~0x3)

/*------------------------------------------------------------------------
 *  newpid  -  Obtain a new (free) process ID
 *------------------------------------------------------------------------
 */
local	pid32	newpid(void)
{
	uint32	i;			/* Iterate through all processes*/
	static	pid32 nextpid = 1;	/* Position in table to try or	*/
					/*   one beyond end of table	*/

	/* Check all NPROC slots */

	for (i = 0; i < NPROC; i++) {
		nextpid %= NPROC;	/* Wrap around to beginning */
		if (proctab[nextpid].prstate == PR_FREE) {
			return nextpid++;
		} else {
			nextpid++;
		}
	}
	return (pid32) SYSERR;
}

syscall srpolicy(int policy)
{
	pageRepPolicy = policy;

	return OK;
}

void dumpfreevmem()
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	memblk	*memptr;	/* Ptr to memory block		*/
	uint32	free_mem;		/* Total amount of free memory	*/

	mask = disable();

	kprintf("PID:%d free memory\n", currpid);

	free_mem = 0;
	for (memptr = proctab[currpid].vMemList.mnext; memptr != NULL;
						memptr = memptr->mnext) {
		free_mem += memptr->mlength;
	}
	kprintf("%10u bytes of free vmemory, %10u bytes allocated.\n"
		"Free list:\n", free_mem, 0x8F000000 - free_mem);
	for (memptr=proctab[currpid].vMemList.mnext; 
		 memptr!=NULL;memptr = memptr->mnext) 
	{
	    kprintf("           [0x%08X to 0x%08X]\r\n",
		(uint32)memptr, ((uint32)memptr) + memptr->mlength - 1);
	}

	restore(mask);
}

/* Virtual memory initialize for vgetmem */
void vmeminit()
{
	struct	memblk	*memptr;	/* Ptr to memory block		*/

	memptr = &proctab[currpid].vMemList;
		
	memptr->mlength = (uint32)(PAGE_NUM_TO_ADDR(589824)) - 
		((uint32)PAGE_NUM_TO_ADDR(4096));

	memptr->mnext = (struct memblk *)(PAGE_NUM_TO_ADDR(4096));
	
	memptr = memptr->mnext;
	memptr->mnext = NULL;
	memptr->mlength = (uint32)(PAGE_NUM_TO_ADDR(589824)) - 
		(uint32)(PAGE_NUM_TO_ADDR(4096));
}

char *vgetmem(int nbytes)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	memblk	*prev, *curr, *leftover;
	struct	memblk	*pVMemList;	/* List of free memory blocks		*/

	mask = disable();
	
	if (nbytes == 0) {
		restore(mask);
		return (char *)SYSERR;
	}

	pVMemList = &proctab[currpid].vMemList;

	if (pVMemList->mlength == 0xFFFFFFFF) {
		vmeminit();
	}

	nbytes = (uint32) roundmb(nbytes);	/* Use memblk multiples	*/

	prev = pVMemList;
	curr = pVMemList->mnext;
	
	while (curr != NULL) {			/* Search free list	*/
		if (curr->mlength == nbytes) {	/* Block is exact match	*/
			prev->mnext = curr->mnext;
			pVMemList->mlength -= nbytes;
			restore(mask);
			return (char *)(curr);

		} else if (curr->mlength > nbytes) { /* Split big block	*/
			leftover = (struct memblk *)((uint32) curr +
					nbytes);
			prev->mnext = leftover;
			leftover->mnext = curr->mnext;
			leftover->mlength = curr->mlength - nbytes;
			pVMemList->mlength -= nbytes;
			restore(mask);
			return (char *)(curr);
		} else {			/* Move to next block	*/
			prev = curr;
			curr = curr->mnext;
		}
	}
	restore(mask);
	return (char *)SYSERR;
}

syscall vfreemem(char *blkaddr, uint32 nbytes)
{
	intmask	mask;			/* Saved interrupt mask		*/
	struct	memblk	*next, *prev, *block;
	struct	memblk	*pVMemList;	/* List of free memory blocks		*/
	uint32	top;
	int i;

	mask = disable();
	if ((nbytes == 0) || 
		((uint32) blkaddr < (uint32)PAGE_NUM_TO_ADDR(4096)) || 
		((uint32) blkaddr >= (uint32)PAGE_NUM_TO_ADDR(59824))) 
	{
		restore(mask);
		return SYSERR;
	}

	nbytes = (uint32) roundmb(nbytes);	/* Use memblk multiples	*/
	block = (struct memblk *)blkaddr;
	
	pVMemList = &proctab[currpid].vMemList;

	prev = pVMemList;			/* Walk along free list	*/
	next = pVMemList->mnext;
	while ((next != NULL) && (next < block)) {
		prev = next;
		next = next->mnext;
	}

	if (prev == pVMemList) {		/* Compute top of previous block*/
		top = (uint32) NULL;
	} else {
		top = (uint32) prev + prev->mlength;
	}

	/* Ensure new block does not overlap previous or next blocks	*/

	if (((prev != pVMemList) && (uint32) block < top)
	    || ((next != NULL)	&& (uint32) block+nbytes>(uint32)next)) {
		restore(mask);
		return SYSERR;
	}

	pVMemList->mlength += nbytes;

	/* Either coalesce with previous block or add to free list */

	if (top == (uint32) block) { 	/* Coalesce with previous block	*/
		prev->mlength += nbytes;
		block = prev;
	} else {			/* Link into list as new node	*/
		block->mnext = next;
		block->mlength = nbytes;
		prev->mnext = block;
	}

	/* Coalesce with next block if adjacent */

	if (((uint32) block + block->mlength) == (uint32) next) {
		block->mlength += next->mlength;
		block->mnext = next->mnext;
	}
#if 0
	kprintf("Free Physical page\n");
	/* Free related physical page if it is active */
	for (i=FIRST_PAGE; i<LAST_PAGE; i++) {
		if (invPageTab[i].pid == currpid) {
			uint32 vAddr = PAGE_NUM_TO_ADDR(invPageTab[i].vpn);

			if (vAddr >= blkaddr && 
				(vAddr + PAGE_SIZE) <= (blkaddr + nbytes))
			{
				kprintf("Free phy page %d vpn:%d\n", i, invPageTab[i].vpn);
				invPageTab[i].active = 0;
			}
		}
	}

	kprintf("Free bs page\n");
	for (i=0; i<MAX_BSTORE; i++) {
		if (bStore[i].pid == currpid) {
			uint32 vAddr = PAGE_NUM_TO_ADDR(bStore[i].vpn);

			if (vAddr >= blkaddr && 
				(vAddr + PAGE_SIZE) <= (blkaddr + nbytes))
			{
				kprintf("Free phy page %d vpn:%d\n", i, bStore[i].vpn);
				bStore[i].pid = -1;
			}
		}
	}

	kprintf("Free done\n");
#endif
	restore(mask);
	return OK;
}

/*------------------------------------------------------------------------
 *  vcreate  -  Create a process to start running a function on x86
 *------------------------------------------------------------------------
 */
pid32 vcreate(int *procaddr, uint32 ssize, int hsize_in_pages, 
	int priority, char *name, uint32 nargs,	...)
{
	uint32		savsp, *pushsp;
	intmask 	mask;    	/* Interrupt mask		*/
	pid32		pid;		/* Stores new process id	*/
	struct	procent	*prptr;		/* Pointer to proc. table entry */
	int32		i;
	uint32		*a;		/* Points to list of args	*/
	uint32		*saddr;		/* Stack address		*/

	mask = disable();
	if (ssize < MINSTK)
		ssize = MINSTK;
	ssize = (uint32) roundew(ssize);
	if (((saddr = (uint32 *)getstk(ssize)) ==
	    (uint32 *)SYSERR ) ||
	    (pid=newpid()) == SYSERR || priority < 1 ) {
		restore(mask);
		return SYSERR;
	}

	prcount++;
	prptr = &proctab[pid];

	/* Initialize process table entry for new process */
	prptr->prstate = PR_SUSP;	/* Initial state is suspended	*/
	prptr->prprio = priority;
	prptr->prstkbase = (char *)saddr;
	prptr->prstklen = ssize;
	prptr->prname[PNMLEN-1] = NULLCH;
	for (i=0 ; i<PNMLEN-1 && (prptr->prname[i]=name[i])!=NULLCH; i++)
		;
	prptr->prsem = -1;
	prptr->prparent = (pid32)getpid();
	prptr->prhasmsg = FALSE;
	prptr->pageDir = initPageData(pid);
	prptr->hsize_in_pages = hsize_in_pages;
	prptr->vMemList.mlength = 0xFFFFFFFF;
	prptr->pTabCnt = getmem(sizeof(uint32) * 1024);
	memset(prptr->pTabCnt, 0, sizeof(uint32) * 1024);

	/* Set up stdin, stdout, and stderr descriptors for the shell	*/
	prptr->prdesc[0] = CONSOLE;
	prptr->prdesc[1] = CONSOLE;
	prptr->prdesc[2] = CONSOLE;

	/* Initialize stack as if the process was called		*/

	*saddr = STACKMAGIC;
	savsp = (uint32)saddr;

	/* Push arguments */
	a = (uint32 *)(&nargs + 1);	/* Start of args		*/
	a += nargs -1;			/* Last argument		*/
	for ( ; nargs > 0 ; nargs--)	/* Machine dependent; copy args	*/
		*--saddr = *a--;	/*   onto created process' stack*/
	*--saddr = (long)INITRET;	/* Push on return address	*/

	/* The following entries on the stack must match what ctxsw	*/
	/*   expects a saved process state to contain: ret address,	*/
	/*   ebp, interrupt mask, flags, registerss, and an old SP	*/

	*--saddr = (long)procaddr;	/* Make the stack look like it's*/
					/*   half-way through a call to	*/
					/*   ctxsw that "returns" to the*/
					/*   new process		*/
	*--saddr = savsp;		/* This will be register ebp	*/
					/*   for process exit		*/
	savsp = (uint32) saddr;		/* Start of frame for ctxsw	*/
	*--saddr = 0x00000200;		/* New process runs with	*/
					/*   interrupts enabled		*/

	/* Basically, the following emulates an x86 "pushal" instruction*/

	*--saddr = 0;			/* %eax */
	*--saddr = 0;			/* %ecx */
	*--saddr = 0;			/* %edx */
	*--saddr = 0;			/* %ebx */
	*--saddr = 0;			/* %esp; value filled in below	*/
	pushsp = saddr;			/* Remember this location	*/
	*--saddr = savsp;		/* %ebp (while finishing ctxsw)	*/
	*--saddr = 0;			/* %esi */
	*--saddr = 0;			/* %edi */
	*pushsp = (unsigned long) (prptr->prstkptr = (char *)saddr);
	restore(mask);
	return pid;
}

