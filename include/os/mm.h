#ifndef INCLUDE_MM_H_
#define INCLUDE_MM_H_
#include "type.h"
#include "sched.h"

#define PGSIZE 4096 // 4KB

void init_page_table();
void do_TLB_Refill();

void do_page_fault();
void init_TLB(void);
void physical_frame_initial(void);

#endif
