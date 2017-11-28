#include <xinu.h>

int proc41()
{
	uint32 *ptr;
	int i;

	ptr = vgetmem(8192);

	*ptr = 11;
	*(ptr + 0x400) = 33;

	kprintf("0x%x %d\n", ptr, *ptr);
	kprintf("0x%x %d\n", ptr+0x400, *(ptr + 0x400));

	sleepms(5000);
	kprintf("0x%x %d\n", ptr, *ptr);
	kprintf("0x%x %d\n", ptr+0x400, *(ptr + 0x400));

	return 0;
}

int proc42()
{
	uint32 *ptr = 0;
	uint32 *ptr2 = 0;
	int i;
	
	ptr = vgetmem(8192);
	
	*ptr = 22;
	*(ptr + 0x400) = 44;
	kprintf("0x%x %d\n", ptr, *ptr);
	kprintf("0x%x %d\n", ptr+0x400, *(ptr + 0x400));

	kprintf("0x%x %d\n", ptr, *ptr);
	kprintf("0x%x %d\n", ptr+0x400, *(ptr + 0x400));

	ptr2 = vgetmem(8192);

	*ptr2 = 66;
	*(ptr2 + 0x400) = 88;
	kprintf("0x%x %d\n", ptr2, *ptr2);
	kprintf("0x%x %d\n", ptr2+0x400, *(ptr2 + 0x400));
	
	kprintf("2:0x%x %d\n", ptr2, *ptr2);
	kprintf("2:0x%x %d\n", ptr2+0x400, *(ptr2 + 0x400));

	return 0;
}

void test41()
{
	resume(vcreate(proc41, 2048, 3, 20, "proc1", 0));
	resume(vcreate(proc42, 2048, 3, 20, "proc2", 0));
}

int proc43()
{
	uint32 *ptr = 0;
	
	ptr = vgetmem(8192);
	
	*ptr = 22;
	*(ptr + 0x400) = 44;
	kprintf("0x%x %d\n", ptr, *ptr);
	kprintf("0x%x %d\n", ptr+0x400, *(ptr + 0x400));

	vfreemem((char *)(ptr+0x400), PAGE_SIZE);
	dumpfreevmem();
	vfreemem((char *)ptr, PAGE_SIZE);
	dumpfreevmem();

	return 0;
}

int proc44()
{
	return 0;
}

void test42()
{
	resume(vcreate(proc43, 2048, 3, 20, "proc1", 0));
	resume(vcreate(proc44, 2048, 3, 20, "proc2", 0));
}

void test4()
{
	test41();
	sleepms(10000);
}
