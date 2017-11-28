#ifndef _SERVICE_H_
#define _SERVICE_H_

extern uint32  pageRepPolicy;

pid32 vcreate(int *procaddr, uint32 ssize, int hsize_in_pages, 
	int priority, char *name, uint32 nargs,	...);
char *vgetmem(int nbytes);
syscall srpolicy(int policy);
syscall vfreemem(char *blkaddr, uint32 nbytes);
void dumpfreevmem();

#endif
