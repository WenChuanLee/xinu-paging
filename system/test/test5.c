#include <xinu.h>

int proc51()
{
	char *ptr = 0;
	
	ptr = vgetmem(20480);
	
	*ptr = 11;
	*(ptr + PAGE_SIZE) = 22;
	*(ptr + PAGE_SIZE * 2) = 33;
	*(ptr + PAGE_SIZE * 3) = 44;
	*(ptr + PAGE_SIZE * 4) = 55;
	kprintf("0x%x %d\n", ptr, *ptr);
	kprintf("0x%x %d\n", ptr + PAGE_SIZE, *(ptr + PAGE_SIZE));
	kprintf("0x%x %d\n", ptr + PAGE_SIZE * 2, *(ptr + PAGE_SIZE * 2));
	kprintf("0x%x %d\n", ptr + PAGE_SIZE * 3, *(ptr + PAGE_SIZE * 3));
	kprintf("0x%x %d\n", ptr + PAGE_SIZE * 4, *(ptr + PAGE_SIZE * 4));
	*(ptr + PAGE_SIZE * 2) = 33;
	*(ptr + PAGE_SIZE * 3) = 44;
	*(ptr + PAGE_SIZE * 4) = 55;
	kprintf("0x%x %d\n", ptr + PAGE_SIZE, *(ptr + PAGE_SIZE));
	
	kprintf("0x%x %d\n", ptr, *ptr);

	*ptr = 11;
	
	*(ptr + PAGE_SIZE * 4) = 55;

	*(ptr + PAGE_SIZE) = 22;

	return 0;
}

int proc52()
{
	char *ptr = vgetmem(12582912);
	int loop = 10;
	int i;

	for (i=0; i<3072; i++) {
		*((uint32 *)(ptr + PAGE_SIZE * i)) = i;
	}

	for (i=0; i<3072; i++) {
		kprintf("->%d\n", *((uint32 *)(ptr + PAGE_SIZE * i)));
	}

	kprintf("%d\n", get_faults());
	return 0;
}
void test51()
{
	resume(vcreate(proc51, 2048, 3, 20, "proc1", 0));
}

void test52()
{
//	resume(vcreate(proc52, 2048, 3, 20, "proc1", 0));
	resume(vcreate(proc52, 2048, 3, 20, "proc1", 0));
}

void test5()
{
	test52();
	sleepms(200000);
}
