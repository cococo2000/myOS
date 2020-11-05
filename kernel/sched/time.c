#include "type.h"
#include "time.h"
#include "sched.h"

uint32_t time_elapsed = 0;

static int MHZ = 300;

uint32_t get_ticks()
{
    return time_elapsed;
}

uint32_t get_timer()
{
    if(current_running->mode == KERNEL_MODE){
        return time_elapsed / (10000000);
    }else{
        // error
        char * input = (char *)0x123456;
        char c = (*input);
    }
}

void latency(uint32_t time)
{
    uint32_t begin_time = get_timer();
    
    while (get_timer() - begin_time < time)
    {
    };
    return;
}