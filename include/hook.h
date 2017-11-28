#ifndef _HOOK_H_
#define _HOOK_H_

void hook_ptable_create(unsigned int pagenum);
void hook_ptable_delete(unsigned int pagenum);
void hook_pfault(char *addr);
void hook_pswap_out(unsigned int pagenum, int framenb);

#endif
