#ifndef _PAGE_H_
#define _PAGE_H_

#define PAGE_SIZE	4096
#define FIRST_PAGE	1024
#define LAST_PAGE	4096
#define FIRST_VPAGE 4096
#define LAST_VPAGE	59824
#define MAX_BSTORE	(8 * 200)

#define IS_PAGE_ALIGN(_addr)	\
	((((uint32)(_addr)) & (PAGE_SIZE - 1)) == 0)

#define ADDR_TO_PAGE_NUM(_addr)	\
	((((uint32)(_addr)) & ~(PAGE_SIZE - 1)) >> 12)

#define PAGE_NUM_TO_ADDR(_pnum)	\
	((_pnum) << 12)

#define IS_PAGE_DIRTY(_pte)	\
	((((uint32)(_pte)) & 0x00000401) == 0x00000401)

typedef struct PageTab {
	unsigned int phyPage[1024];
} PageTab;

typedef struct PageDir {
	PageTab *pTab[1024];
} PageDir;

typedef struct InvPageTab {
	pid32 pid;
	uint32 vpn;
	/* Pointer to page table entry */
	uint32 *pte;
	uint32 active;
} InvPageTab;

typedef struct BStore {
	pid32 pid;
	uint32 vpn;
	/* Pointer to page table entry */
	uint32 *pte;
} BStore;

extern PageDir *toFreePageDir;		/*  */
extern InvPageTab invPageTab[LAST_PAGE];
extern BStore bStore[MAX_BSTORE];
extern uint32 pageFaultNum;

PageDir *initPageData(pid32 pid);
void freePageData(PageDir *pDir);
char *getPageMem(pid32 pid);
syscall	freePageMem(char *pageAddr);
void dumpFreePage();
void initialize_paging();
void setCR3(pid32 pid);
uint32 getPhyPage(PageDir *pDir, uint32 vAddr);
uint32* getPhyPagePtr(PageDir *pDir, uint32 vAddr);
void invalidateAllPages();
void invalidateSinglePage(void *m);
void pagefaultHandler(uint32 cc);
void pagefault();
void initBStore();
int get_faults();

#endif
