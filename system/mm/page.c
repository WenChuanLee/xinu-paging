#include <xinu.h>

//#define LOG_PAGE_C
//#define LOGGING_ON

PageDir *toFreePageDir = NULL;
InvPageTab invPageTab[LAST_PAGE];
BStore bStore[MAX_BSTORE];
/* FIFO queue for page replacement */
uint32 pageQ[LAST_PAGE], pQH, pQT;
uint32 pageFaultNum = 0;

void dumpPageTab(PageTab *pTab)
{
	int i;

	kprintf("Page Table:0x%x\n", pTab);
	for (i=0; i<5; i++) {
		kprintf("ent:%4d 0x%x\n", i, pTab->phyPage[i]);
	}
}

void initBStore()
{
	int i;

#ifdef LOG_PAGE_C
	kprintf("Initialize backing store\n");
#endif
	/* Initialize all backing store, this is shared by all processes */
	for (i=0; i<MAX_BSTORE; i++) {
		if ((i % 200) == 0) {
			bsd_t bs = allocate_bs(200);
			if (bs == -1) {
				kprintf("Fatal error: backing store %d allocation error\n", 
					i / 200);
				panic("");
			}
#ifdef LOG_PAGE_C
			kprintf("bs %d allocated\n", bs);
#endif
		}
		bStore[i].pid = -1;
	}
}

/* Lab5-Bonus: Global clock */
uint32 pGClk()
{
	static int prevStop;
	uint32 phyPageNum = 0;

	while (phyPageNum == 0) {
		int i;
	
		for (i=prevStop; i<LAST_PAGE; i++) {
			uint32 *pPte;
			if (invPageTab[i].pid == -1)
				continue;

			if (invPageTab[i].vpn < LAST_PAGE)
				continue;
		
			pPte = invPageTab[i].pte;
			
			/* Found a page */
			if (((*pPte) & 0x60) == 0) {
				phyPageNum = i;
				prevStop = phyPageNum + 1;
				break;
			} else if (((*pPte) & 0x60) == 0x20) {
				(*pPte) &= 0xFFFFFFDF;
			} else if (((*pPte) & 0x60) == 0x60) {
				(*pPte) &= 0xFFFFFFBF;
			} else {
				kprintf("Fatal Error: undefined table entry value:0x%x\n",
					*pPte);
			}
		}
		if (phyPageNum == 0)
			prevStop = FIRST_PAGE;
	}

	return phyPageNum;
}

int searchBSPageNum(pid32 pid, uint32 vpn)
{
	int i;

	for (i=0; i<MAX_BSTORE; i++) {
		if (bStore[i].pid == pid && bStore[i].vpn == vpn) {
			return i;
		}
	}

	return -1;
}

int getBSPageNum()
{
	int i;

	for (i=0; i<MAX_BSTORE; i++) {
		if (bStore[i].pid == -1) {
			return i;
		}
	}

	return -1;
}

void freeBSPageNum(int pageNum)
{
	intmask	mask;			/* Saved interrupt mask		*/

	mask = disable();
	
	bStore[pageNum].pid = -1;

	restore(mask);
}

/* Output free pages */
void dumpFreePage()
{
	uint32 freePages = 0;

	int i;

	for (i=0; i<LAST_PAGE; i++) {
		if (invPageTab[i].pid == -1) {
			freePages++;
		}
	}

	kprintf("Free Pages:%d mem:%u\n", freePages, freePages << 12);
}

int searchFreePhyPageNum()
{
	int i;

	for (i=FIRST_PAGE; i<LAST_PAGE; i++) {
		if (invPageTab[i].pid == -1) {
			return i;
		}
	}

	return 0;
}

void pEnQNum(uint32 phyPageNum)
{
	pageQ[pQT] = phyPageNum;
	
	pQT++;
	pQT %= LAST_PAGE;
}

uint32 pDeQNum()
{
	uint32 ret;

	ret = pageQ[pQH];
	
	pQH++;
	pQH %= LAST_PAGE;

	return ret;
}

/*------------------------------------------------------------------------
 *  getPageMem  -  Allocate memory for page, returning lowest word address
 *------------------------------------------------------------------------
 */
char *getPageMem(pid32 pid)
{
	intmask	mask;			/* Saved interrupt mask		*/
	uint32 pageNum;
	char* pAddr;

	mask = disable();
	
	pageNum = searchFreePhyPageNum();

	if (pageNum == 0) {
		return (char *)SYSERR;
	}

	invPageTab[pageNum].pid = pid;
	invPageTab[pageNum].vpn = pageNum;
	invPageTab[pageNum].active = 1;

	pAddr = (char *) PAGE_NUM_TO_ADDR(pageNum);
	
	restore(mask);

	return pAddr;
}

/*------------------------------------------------------------------------
 *  freePageMem  -  Free a physical page
 *------------------------------------------------------------------------
 */
syscall	freePageMem(char *pageAddr)
{
	intmask	mask;			/* Saved interrupt mask		*/

	mask = disable();
	
	if (!IS_PAGE_ALIGN(pageAddr))
		return SYSERR;

	invPageTab[ADDR_TO_PAGE_NUM(pageAddr)].pid = -1;

	restore(mask);
	
	return OK;
}

void freePageTable(pid32 pid, uint32 pdeIdx)
{
	proctab[pid].pTabCnt[pdeIdx]--;

	if (proctab[pid].pTabCnt[pdeIdx] == 0) {
		uint32 tabAddr = 
			(uint32)proctab[pid].pageDir->pTab[pdeIdx];

		proctab[pid].pageDir->pTab[pdeIdx] = 
			(PageTab *)(tabAddr & 0xFFFFFFFE);

		freePageMem((char *)(tabAddr & 0xFFFFF000));

#ifdef LOGGING_ON
		hook_ptable_delete(pdeIdx);
#endif
	}
}

/* Get free physical frame by swapping out other pages */
char *getPageMemBySwap(pid32 inPid, uint32 inPdeIdx, int *sameSwap)
{
	int outPageNum = (pageRepPolicy == FIFO)? pDeQNum() : pGClk();
	char *outPAddr = (char *)PAGE_NUM_TO_ADDR(outPageNum);
	pid32 outPid = invPageTab[outPageNum].pid;
	uint32 outVpn = invPageTab[outPageNum].vpn;
	uint32 outVAddr = PAGE_NUM_TO_ADDR(outVpn);
	uint32 outPdeIdx = outVAddr >> 22;
	uint32 *pOutPte = getPhyPagePtr(proctab[outPid].pageDir, outVAddr);	
	char *phyPage;

	/* This page is still being used by other process */
	/* Write to backing store if page is dirty */
	if (invPageTab[outPageNum].active) {
		int newBsPageNum = getBSPageNum();
		bsd_t bs = newBsPageNum / 200;
		uint32 offset = newBsPageNum % 200;
		
		/* Record info: Phy(outPage) uses BS(newBsPageNum) */
		bStore[newBsPageNum].pid = outPid;
		bStore[newBsPageNum].vpn = outVpn;

		while (write_bs(outPAddr, bs, offset) == SYSERR) {
			;
		}
#ifdef LOGGING_ON
		hook_pswap_out(outVpn, outPageNum);
#endif
	}
		
	/* Mark page entry as invalid */
	*pOutPte = *pOutPte & 0xFFFFFFFE;
	
	/* This page has been swapped out, now we can use it */
	phyPage = (char *)PAGE_NUM_TO_ADDR(outPageNum);

	/* 
	 * One entry is not present, so do corresponding task
	 */
	invalidateSinglePage((void *)outVAddr);
	//invalidateAllPages();
	
	if (outPid == inPid && inPdeIdx == outPdeIdx)
		*sameSwap = 1;

	/* We may need to free the page table */
	if (!(*sameSwap))
		freePageTable(outPid, outPdeIdx);

	return phyPage;
}


/* 
 * Allocate the whole page structure including page dir and page table 
 * This is called when a new process is being created.
 */
PageDir* initPageData(pid32 pid)
{
	PageDir *pDir = NULL;
	PageTab *pTab = NULL;
	int i;
	int sameSwap;

	/* Get page directory */
	pDir = (PageDir *)getPageMem(pid);
	
	/* Not 4096-bytes aligned */
	if (pDir == (PageDir *)SYSERR) {
		pDir = (PageDir *)getPageMemBySwap(pid, 0, &sameSwap);

		if (pDir == (PageDir *)SYSERR) {
			kprintf("Fatal Error: Cannot allocate memory for page dir\n");
			goto bail;
		}
		invPageTab[ADDR_TO_PAGE_NUM(pDir)].pid = pid;
		invPageTab[ADDR_TO_PAGE_NUM(pDir)].vpn = ADDR_TO_PAGE_NUM(pDir);
		invPageTab[ADDR_TO_PAGE_NUM(pDir)].active = 1;
	}
	memset(pDir, 0, sizeof(PageDir));
#ifdef LOG_PAGE_C
	kprintf("Page directory:0x%x\n", pDir);
#endif
	
	/* Page dir entry initialization */
	for (i=0; i<5; i++) {
		int pdeIdx = (i < 4) ? i : 576;
		
		pTab = (PageTab *) getPageMem(pid);
		
		if (pTab == (PageTab *)SYSERR) {
			pTab = (PageTab *)getPageMemBySwap(pid, pdeIdx, &sameSwap);

			if (pTab == (PageTab *)SYSERR) {
				kprintf("Fatal Error: Cannot allocate memory for page table\n");
				goto bail;
			}
			invPageTab[ADDR_TO_PAGE_NUM(pTab)].pid = pid;
			invPageTab[ADDR_TO_PAGE_NUM(pTab)].vpn = ADDR_TO_PAGE_NUM(pTab);
			invPageTab[ADDR_TO_PAGE_NUM(pTab)].active = 1;
		}
		if (IS_PAGE_ALIGN(pTab)) {
			int j;

			/* Global Page should be present */
			pDir->pTab[pdeIdx] = (PageTab *)(((unsigned int) pTab) | 0x3);
#ifdef LOG_PAGE_C
			kprintf("Page dir index:%d, paddr:0x%x\n", 
				pdeIdx, pDir->pTab[pdeIdx]);
#endif
#ifdef LOGGING_ON
			hook_ptable_create(pdeIdx);
#endif
			/* Page initialization */
			for (j=0; j<1024; j++) {
				pTab->phyPage[j] = (pdeIdx << 22) | (j << 12) | 0x3;
			}
		} else
			goto bail;
	}

	return pDir;
bail:
	kprintf("Fatal Error:\n");
	kprintf("Page directory:0x%x\n", pDir);
	kprintf("Page table:0x%x\n", pTab);
	panic("");

	return NULL;
}

/* 
 * Free the whole page structure including page dir and page table 
 * This is called when a process is being freed.
 */
void freePageData(PageDir *pDir)
{
	int i;

	for (i=0; i<1024; i++) {
		uint32 pde = (uint32) pDir->pTab[i];
		/* Page table is present */
		if (pde & 0x1) {
			if (freePageMem((char *)(pde & 0xFFFFF000)) == SYSERR) {
				kprintf("Fatal error when free page table:0x%x",
					pde);
				goto bail;
			}
		}
	}
	if (freePageMem((char *)(((uint32)pDir) & 0xFFFFF000)) == SYSERR) {
		kprintf("Fatal error when free page dir:0x%x", pDir);
		goto bail;
	}

	return;
bail:
	panic("");

}

/* Set CR3 according to pid */
void setCR3(pid32 pid)
{
	uint32 pdAddr;

#ifdef LOG_PAGE_C
	kprintf("Set CR3 with pid%d's page dir:0x%x\n",
		pid, proctab[pid].pageDir);
#endif

	if (!proctab[pid].pageDir) {
		kprintf("Fatal Error:\n"
			"Setting CR3 to pid%d's page dir\n");
		panic("");
	}

	pdAddr = (uint32)proctab[pid].pageDir;

	asm("mov %0, %%eax;"
		"mov %%eax, %%cr3;"
		:
        :"r"(pdAddr)
        :"%eax");

#ifdef LOG_PAGE_C
	kprintf("CR3 set\n");
#endif
}

/* Enable paging */
void initialize_paging()
{
	int i;

#ifdef LOG_PAGE_C
	kprintf("Initialize paging\n");
	kprintf("Initialize inverted page table\n");
#endif
	/* Initialize all free pages */
	for (i=FIRST_PAGE; i<LAST_PAGE; i++) {
		invPageTab[i].pid = -1;
	}

	/* Initialize page queue */
	pQH = pQT = 0;

	/* Initialize prnull's page data */
	proctab[0].pageDir = initPageData(0);

	/* Set prnull's page directory address to CR3 */
	setCR3(0);

	/* Install page fault interrupt handler */
	set_evec(0xE, (uint32)pagefault);	

#ifdef LOG_PAGE_C
	kprintf("Eanbling paging...\n");
#endif

	/* Enable Paging */
	asm("mov %%cr0, %%eax;"
		"or $0x80000000, %%eax;"
		"mov %%eax, %%cr0;"
		:
        :
        :"%eax"
		);
#ifdef LOG_PAGE_C
	kprintf("Paging enabled\n");
#endif
}

uint32 getPhyPage(PageDir *pDir, uint32 vAddr)
{
	return *getPhyPagePtr(pDir, vAddr);
}

uint32* getPhyPagePtr(PageDir *pDir, uint32 vAddr)
{
	PageTab *pTab;
	uint32 *pPage;
	uint32 pdeIdx = vAddr >> 22;
	uint32 pteIdx = (vAddr >> 12) & 0x3FF;

	pTab = (PageTab *)((uint32)(pDir->pTab[pdeIdx]) & 0xFFFFF000);

	pPage = &pTab->phyPage[pteIdx];

	return pPage;
}

void invalidateAllPages()
{
	asm("movl %%cr3, %%eax;"
		"movl %%eax, %%cr3;"
		:
        :
        :"%eax"
		);
}

/* Invalidate single page */
void invalidateSinglePage(void *m)
{
	asm volatile("invlpg (%0)" : : "b"(m) : "memory");
}

int get_faults()
{
	return pageFaultNum;
}

/* 
 * This is the main page fault handler called by .S page fault handler
 * No interrupt mask because it will only be
 * called inside interrupt handler 
 */
void pagefaultHandler(uint32 cc)
{
	uint32 vAddr = 0;
	uint32 vpn;
	char *phyPage;
	uint32 pdeIdx;
	uint32 pteIdx;
	PageDir *pDir;
	PageTab *pTab;
	int bsPageNum;
	int sameSwap = 0;

	asm("movl %%cr2, %0;"
		:"=r"(vAddr)
        :
        :
		);

#ifdef LOG_PAGE_C
	kprintf("Handling page fault of pid:%d\n", currpid);
	kprintf("Condition code:[P=%x R/W=%x U/S=%x]\n",
		cc & 0x1, (cc & 0x2) >> 1, (cc & 0x4) >> 1);

	kprintf("Virtual address:0x%x\n", vAddr);
#endif

#ifdef LOGGING_ON
	hook_pfault((char *)vAddr);
#endif

	vpn = ADDR_TO_PAGE_NUM(vAddr);
	
	if (vpn < LAST_PAGE || vpn >= LAST_VPAGE) {
		kprintf("Invalid virtual page:%d, kill process:%d\n", vpn, currpid);
		kill(currpid);
		return;
	}

	pdeIdx = vAddr >> 22;
	pteIdx = (vAddr >> 12) & 0x3FF;
	pDir = proctab[currpid].pageDir;
	pTab = pDir->pTab[pdeIdx];
	bsPageNum = searchBSPageNum(currpid, vpn);

//	kprintf("pdeIdx:0x%d pteIdx:0x%x\n", pdeIdx, pteIdx);
	/* Allocate and initialize page table for it if not exist */
	if (!(((uint32)pTab) & 0x1)) {
		pTab = (PageTab *)getPageMem(currpid);

		if (pTab == (PageTab *)SYSERR) {
			pTab = (PageTab *)getPageMemBySwap(currpid, pdeIdx, &sameSwap);

			if (pTab == (PageTab *)SYSERR) {
				kprintf("Fatal Error: Cannot allocate memory for page table\n");
				kprintf("Fatal Error: kill process %d\n", currpid);
				kill(currpid);
			}
			invPageTab[ADDR_TO_PAGE_NUM(pTab)].pid = currpid;
			invPageTab[ADDR_TO_PAGE_NUM(pTab)].vpn = ADDR_TO_PAGE_NUM(pTab);
			invPageTab[ADDR_TO_PAGE_NUM(pTab)].active = 1;
		}
		
		pDir->pTab[pdeIdx] = (PageTab *)(((uint32)pTab) | 0x3);
		memset(pTab, 0, sizeof(PageTab));

#ifdef LOGGING_ON
		hook_ptable_create(pdeIdx);
#endif
	} 	

	/* Don't forget to mask the final 12 bit out */
	pTab = (PageTab *)(((uint32)pTab) & 0xFFFFF000);
  /* Need to get a free physical page */
  phyPage = getPageMem(currpid);

	/* Swap out a physical page to backing store */
	if (phyPage == (char *)SYSERR) {
		phyPage = getPageMemBySwap(currpid, pdeIdx, &sameSwap);
	} 

	/* Swap in the target page if it was swapped out */
	if (bsPageNum >= 0) {
		bsd_t bs = bsPageNum / 200;
		uint32 offset = bsPageNum % 200;
		while (read_bs(phyPage, bs, offset) == SYSERR) {
			;
		}
		bStore[bsPageNum].pid = -1;
	}

	/* Mark page entry as present in pTab */
	pTab->phyPage[pteIdx] = ((uint32)phyPage) | 0x3;
	if (pageRepPolicy == FIFO)
		pEnQNum(ADDR_TO_PAGE_NUM(phyPage));
	invPageTab[ADDR_TO_PAGE_NUM(phyPage)].pid = currpid;
	invPageTab[ADDR_TO_PAGE_NUM(phyPage)].vpn = vpn;
	invPageTab[ADDR_TO_PAGE_NUM(phyPage)].pte = &pTab->phyPage[pteIdx];
	/* 
	 * pTab have at least one entry exist in physical frame,
	 * so increment
	 */
	if (!sameSwap)
		proctab[currpid].pTabCnt[pdeIdx]++;

	pageFaultNum++;
}
