#include <xinu.h>

void test34()
{
	uint32 tmp;

	asm("mov $0xdeadbeef, %%eax;"
		"pusha;"
		:
        :
        :);
	
	asm("mov $0x0, %%eax;"
		"popa;"
		"mov %%eax, %0"
		:"=r"(tmp)
        :
        :);

	kprintf("----->0x%x\n", tmp);
}

static void proc1()
{
	uint32 *pAddr1 = (uint32 *)0x1000000;
	uint32 *pAddr2 = (uint32 *)0x1400000;
	uint32 *pAddr3 = (uint32 *)0x1800000;
	uint32 *pAddr4 = (uint32 *)0x1A00000;

	kprintf("P1:PF1 of 0x%x\n", pAddr1);
	*pAddr1 = 11;
	kprintf("p11:%u\n", *pAddr1);
	
	kprintf("P1:PF2 of 0x%x\n", pAddr2);
	*pAddr2 = 22;
	kprintf("pAddr2:0x%x content:%u\n", pAddr2, *pAddr2);

	kprintf("P1:PF3 of 0x%x\n", pAddr3);
	*pAddr3 = 33;
	kprintf("p11:%u\n", *pAddr3);
	
	kprintf("P1:PF4 of 0x%x\n", pAddr4);
	*pAddr4 = 44;
	kprintf("pAddr2:0x%x content:%u\n", pAddr4, *pAddr4);

	sleepms(2000);

	kprintf("P1:PF5 of 0x%x\n", pAddr1);
	kprintf("pAddr1:0x%x content:%u\n", pAddr1, *pAddr1);
	
	kprintf("P1:PF6 of 0x%x\n", pAddr2);
	kprintf("pAddr2:0x%x content:%u\n", pAddr2, *pAddr2);
	
	kprintf("P1:PF7 of 0x%x\n", pAddr3);
	kprintf("pAddr3:0x%x content:%u\n", pAddr3, *pAddr3);
	
	kprintf("P1:PF8 of 0x%x\n", pAddr4);
	kprintf("pAddr4:0x%x content:%u\n", pAddr4, *pAddr4);

}

static void proc2()
{
	uint32 *pAddr1 = (uint32 *)0x1000000;
	uint32 *pAddr2 = (uint32 *)0x1400000;
	uint32 *pAddr3 = (uint32 *)0x1800000;
	uint32 *pAddr4 = (uint32 *)0x1A00000;
	
	kprintf("P2:PF1 of 0x%x\n", pAddr1);
	*pAddr1 = 55;
	kprintf("p11:%u\n", *pAddr1);
	
	kprintf("P2:PF2 of 0x%x\n", pAddr2);
	*pAddr2 = 66;
	kprintf("pAddr2:0x%x content:%u\n", pAddr2, *pAddr2);

	kprintf("P2:PF3 of 0x%x\n", pAddr3);
	*pAddr3 = 77;
	kprintf("p11:%u\n", *pAddr3);
	
	kprintf("P2:PF4 of 0x%x\n", pAddr4);
	*pAddr4 = 88;
	kprintf("pAddr2:0x%x content:%u\n", pAddr4, *pAddr4);

	kprintf("P2:PF5 of 0x%x\n", pAddr1);
	kprintf("pAddr1:0x%x content:%u\n", pAddr1, *pAddr1);
	
	kprintf("P2:PF6 of 0x%x\n", pAddr2);
	kprintf("pAddr2:0x%x content:%u\n", pAddr2, *pAddr2);
	
	kprintf("P2:PF7 of 0x%x\n", pAddr3);
	kprintf("pAddr3:0x%x content:%u\n", pAddr3, *pAddr3);
	
	kprintf("P2:PF8 of 0x%x\n", pAddr4);
	kprintf("pAddr4:0x%x content:%u\n", pAddr4, *pAddr4);

}

static void proc3()
{
	uint32 *pAddr1 = (uint32 *)0x1000000;
	uint32 *pAddr2 = (uint32 *)0xA4000000;

	kprintf("P3:PF1 of 0x%x\n", pAddr1);
	*pAddr1 = 31;
	kprintf("p31:%u\n", *pAddr1);
	
	kprintf("P3:PF2 of 0x%x\n", pAddr2);
	*pAddr2 = 32;
	kprintf("pAddr2:0x%x content:%u\n", pAddr2, *pAddr2);
	
	kprintf("P3:PF3 of 0x%x\n", pAddr1);
	kprintf("pAddr1:%u\n", pAddr1, *pAddr1);
	
	kprintf("P3:PF4 of 0x%x\n", pAddr2);
	kprintf("pAddr2:0x%x content:%u\n", pAddr2, *pAddr2);

	sleepms(4000);
	kprintf("P3:PF5 of 0x%x\n", pAddr1);
	kprintf("pAddr1:0x%x content:%u\n", pAddr1, *pAddr1);
	
	kprintf("P3:PF6 of 0x%x\n", pAddr2);
	kprintf("pAddr2:0x%x content:%u\n", pAddr2, *pAddr2);
}

void test33()
{
	resume(create(proc1, 4096, 20, "proc1", 0));
	resume(create(proc2, 4096, 20, "proc2", 0));
}

void test32()
{
	uint32 *vaddr1 = (uint32 *)0x1000000;
	uint32 *vaddr2 = (uint32 *)0x1400000;
	
	kprintf("Test 31\n");

	kprintf("PF1 of 0x%x\n", vaddr1);
	*vaddr1 = 123;
	kprintf("vaddr1:0x%x content:%u\n", vaddr1, *vaddr1);

	kprintf("PF2 of 0x%x\n", vaddr2);
	*vaddr2 = 456;
	kprintf("vaddr2:0x%x content:%u\n", vaddr2, *vaddr2);

	kprintf("PF3 of 0x%x\n", vaddr1);
	kprintf("vaddr1:0x%x content:%u\n", vaddr1, *vaddr1);
	
	kprintf("PF4 of 0x%x\n", vaddr2);
	kprintf("vaddr1:0x%x content:%u\n", vaddr2, *vaddr2);
}

void test31()
{
	uint32 *vaddr = (uint32 *)0x1000000;

	/* Test 3 */
	kprintf("Test 31\n");

	kprintf("PF1\n");
	kprintf("vaddr:0x%x content:%u\n", vaddr, *vaddr);

	*vaddr = 123;
	
	kprintf("PF2\n");
	kprintf("vaddr:0x%x content:%u\n", vaddr+0x400, *(vaddr+0x400));

	kprintf("PF3\n");
	memset(vaddr, 0, 4096);
	kprintf("vaddr:0x%x content:%u\n", vaddr, *vaddr);

	kprintf("PF4\n");
	*(vaddr+0x400) = 456;
	kprintf("vaddr:0x%x content:%u\n", vaddr+0x400, *(vaddr+0x400));

	kprintf("PF5\n");
	kprintf("vaddr:0x%x content:%u\n", vaddr, *vaddr);

	kprintf("PF6\n");
	kprintf("vaddr:0x%x content:%u\n", vaddr+0x400, *(vaddr+0x400));

	kprintf("Test 31 passed\n");
}

void test3()
{
	test33();

	sleepms(10000);
}
