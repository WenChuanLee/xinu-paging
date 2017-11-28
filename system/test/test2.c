#include <xinu.h>

void test2()
{
	int i;
	PageDir *pDir;
	uint32 *pPage;
	uint32 vaddr;
	uint32 tmp;

	kprintf("Begin test2\n");

	pDir = proctab[currpid].pageDir;

	kprintf("Dump pid:%d name:%s's page dir:%x\n",
		currpid, proctab[currpid].prname, pDir);

	for (i=0; i<5; i++) {
		int pdeIdx = (i < 4) ? i : 576;
		uint32 pde;

		/* Global Page should be present */
		pde = (uint32)pDir->pTab[pdeIdx];
		kprintf("Page dir index:%d, paddr:0x%x\n", 
				pdeIdx, pde);
		pde &= 0xFFFFF000;
		/* Page */
		kprintf("Page:%d, paddr:0x%x\n", 
				512, ((PageTab *)pde)->phyPage[512]);
	}

	//invalidateAllPages();
	
	kprintf("Test2 done\n");
}
