#include "mm.h"

void init_page_table()
{
    int i, j;
    uint64_t PFN  = 0x20000;
    for (i = 0; i < NUM_MAX_TASK; i++) {
        for (j = 0; j < NUM_MAX_PTE; j++) {
            pcb[i].page_table[j].entrylo0 = (PFN << 6) | (PTE_C << 3) | (PTE_D << 2) | (PTE_V << 1) | PTE_G;
            PFN += 1;
            pcb[i].page_table[j].entrylo1 = (PFN << 6) | (PTE_C << 3) | (PTE_D << 2) | (PTE_V << 1) | PTE_G;
            PFN += 1;
        }
    }
}
void do_TLB_Refill()
{

}

void do_page_fault()
{
}

void init_TLB(void)
{
    int i = 0;
    uint64_t VPN2 = 0;
    uint64_t PFN  = 0x20000;

    set_cp0_pagemask(0);
    for (i = 0; i < NUM_MAX_TLB; i++) {
        set_cp0_entryhi(VPN2 << 13);
        VPN2 += 1;
        set_cp0_entrylo0((PFN << 6) | (PTE_C << 3) | (PTE_D << 2) | (PTE_V << 1) | PTE_G);
        PFN += 1;
        set_cp0_entrylo1((PFN << 6) | (PTE_C << 3) | (PTE_D << 2) | (PTE_V << 1) | PTE_G);
        PFN += 1;
        set_cp0_index(i);
        tlbwi_operation();
    }
}

void physical_frame_initial(void)
{

}