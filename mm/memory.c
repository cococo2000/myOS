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
    uint32_t entrylo0, entrylo1;
    uint64_t context = get_cp0_context();
    tlbp_operation();
    uint32_t index = 0;
    if (get_cp0_index() & 0x80000000) {
        // TLB refill
        set_cp0_index(index);
        index = (index + 1) % NUM_MAX_TLB;
    }
    else {
        // TLB invalid
        if(!(current_running->page_table[context >> 4 & (NUM_MAX_PTE - 1)].entrylo0 & 0x2) ||
           !(current_running->page_table[context >> 4 & (NUM_MAX_PTE - 1)].entrylo1 & 0x2)){
            do_page_fault();
        }
    }
    entrylo0 = current_running->page_table[context >> 4 & (NUM_MAX_PTE - 1)].entrylo0;
    entrylo1 = current_running->page_table[context >> 4 & (NUM_MAX_PTE - 1)].entrylo1;
    set_cp0_entrylo0(entrylo0);
    set_cp0_entrylo1(entrylo1);
    tlbwi_operation();
}

void do_page_fault()
{
    static uint64_t PFN  = 0x20000;
    uint64_t context = get_cp0_context();
    current_running->page_table[context >> 4 & (NUM_MAX_PTE - 1)].entrylo0 = (PFN << 6) | (PTE_C << 3) | (PTE_D << 2) | (PTE_V << 1);// | PTE_G;
    PFN ++;
    current_running->page_table[context >> 4 & (NUM_MAX_PTE - 1)].entrylo1 = (PFN << 6) | (PTE_C << 3) | (PTE_D << 2) | (PTE_V << 1);// | PTE_G;
    PFN ++;
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