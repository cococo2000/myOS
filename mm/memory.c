#include "mm.h"

void init_page_table()
{

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
    for (i = 0; i < 64; i++) {
        set_cp0_entryhi(VPN2 << 13);
        VPN2 += 1;
        set_cp0_entrylo0(PFN << 6 | 0b010111);
        PFN += 1;
        set_cp0_entrylo1(PFN << 6 | 0b010111);
        PFN += 1;
        set_cp0_index(i);
        tlbwi_operation();
    }
}

void physical_frame_initial(void)
{

}