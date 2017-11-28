#include <xinu.h>

void test1()
{
	int i;

	kprintf("Begin test1\n");
	kprintf("test1 passed\n");
	
	return;
bail:
	kprintf("Test Failed\n");
	dumpFreePage();
	panic("");

}
