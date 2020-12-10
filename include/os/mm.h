#ifndef INCLUDE_MM_H_
#define INCLUDE_MM_H_
#include "type.h"
#include "sched.h"

#define PAGE_SIZE 4096 // 4KB
#define NUM_MAX_TLB 64

void init_page_table();
void do_TLB_Refill();

void do_page_fault();
void init_TLB(void);
void physical_frame_initial(void);

#endif
