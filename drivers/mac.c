#include "mac.h"
#include "irq.h"
#include "sched.h"

queue_t recv_block_queue;
uint32_t recv_flag[PNUM] = {0};
uint32_t ch_flag = 0;
uint32_t mac_cnt = 0;

desc_t tx_descriptor[NUM_DMA_DESC];
desc_t rx_descriptor[NUM_DMA_DESC];

char send_buf[NUM_DMA_DESC * PSIZE * 4];
char recv_buf[NUM_DMA_DESC * PSIZE * 4];
uint64_t buf_length[NUM_DMA_DESC] = {0};

uint32_t reg_read_32(uint64_t addr)
{
    return *((uint32_t *)addr);
}
uint32_t read_register(uint64_t base, uint64_t offset)
{
    uint64_t addr = base + offset;
    uint32_t data;

    data = *(volatile uint32_t *)addr;
    return data;
}

void reg_write_32(uint64_t addr, uint32_t data)
{
    *((uint32_t *)addr) = data;
}

static void s_reset(mac_t *mac) //reset mac regs will inrupt
{
    uint32_t time = 1000000;
    reg_write_32(mac->dma_addr, 0x01);

    while ((reg_read_32(mac->dma_addr) & 0x01))
    {
        reg_write_32(mac->dma_addr, 0x01);
        while (time)
        {
            time--;
        }
    };
}

static void gmac_get_mac_addr(uint8_t *mac_addr)
{
    uint32_t addr;

    addr = read_register(GMAC_BASE_ADDR, GmacAddr0Low);
    mac_addr[0] = (addr >> 0) & 0x000000FF;
    mac_addr[1] = (addr >> 8) & 0x000000FF;
    mac_addr[2] = (addr >> 16) & 0x000000FF;
    mac_addr[3] = (addr >> 24) & 0x000000FF;

    addr = read_register(GMAC_BASE_ADDR, GmacAddr0High);
    mac_addr[4] = (addr >> 0) & 0x000000FF;
    mac_addr[5] = (addr >> 8) & 0x000000FF;
}

#if 1

/* print DMA regs */
void print_dma_regs()
{
    uint32_t regs_val1, regs_val2;

    printk(">>[DMA Register]\n");

    // [0] Bus Mode Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaBusMode);

    printk("  [0] Bus Mode : 0x%x, ", regs_val1);

    // [3-4] RX/TX List Address Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaRxBaseAddr);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaTxBaseAddr);
    printk("  [3-4] TX/RX : 0x%x/0x%x\n", regs_val2, regs_val1);

    // [5] Status Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaStatus);
    printk("  [5] Status : 0x%x, ", regs_val1);

    // [6] Operation Mode Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaControl);
    printk("  [6] Control : 0x%x\n", regs_val1);

    // [7] Interrupt Enable Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaInterrupt);
    printk("  [7] Interrupt : 0x%x, ", regs_val1);

    // [8] Miss
    regs_val1 = read_register(DMA_BASE_ADDR, DmaMissedFr);
    printk("  [8] Missed : 0x%x\n", regs_val1);

    // [18-19] Current Host TX/RX Description Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaTxCurrDesc);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaRxCurrDesc);
    printk("  [18-19] Current Host TX/RX Description : 0x%x/0x%x\n", regs_val1, regs_val2);

    // [20-21] Current Host TX/RX Description Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaTxCurrAddr);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaRxCurrAddr);
    printk("  [20-21] Current Host TX/RX Buffer Address : 0x%x/0x%x\n", regs_val1, regs_val2);
}

/* print DMA regs */
void print_mac_regs()
{
    printk(">>[MAC Register]\n");
    uint32_t regs_val1, regs_val2;
    uint8_t mac_addr[6];

    // [0] MAC Configure Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacConfig);
    printk("  [0] Configure : 0x%x, ", regs_val1);

    // [1] MAC Frame Filter
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacFrameFilter);
    printk("  [1] Frame Filter : 0x%x\n", regs_val1);

    // [2-3] Hash Table High/Low Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacHashHigh);
    regs_val2 = read_register(GMAC_BASE_ADDR, GmacHashLow);
    printk("  [2-3] Hash Table High/Low : 0x%x-0x%x\n", regs_val1, regs_val2);

    // [6] Flow Control Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacFlowControl);
    printk("  [6] Flow Control : 0x%x, ", regs_val1);

    // [8] Version Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacVersion);
    printk("  [8] Version : 0x%x\n", regs_val1);

    // [14] Interrupt Status Register and Interrupt Mask
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacInterruptStatus);
    regs_val2 = read_register(GMAC_BASE_ADDR, GmacInterruptMask);
    printk("  [14-15] Interrupt Status/Mask : 0x%x/0x%x\n", regs_val1, regs_val2);

    // MAC address
    gmac_get_mac_addr(mac_addr);
    printk("  [16-17] Mac Addr : %X:%X:%X:%X:%X:%X\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
}

void printf_dma_regs()
{
    uint32_t regs_val1, regs_val2;

    printf(">>[DMA Register]\n");

    // [0] Bus Mode Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaBusMode);

    printf("  [0] Bus Mode : 0x%x, ", regs_val1);

    // [3-4] RX/TX List Address Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaRxBaseAddr);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaTxBaseAddr);
    printf("  [3-4] RX/TX : 0x%x/0x%x\n", regs_val1, regs_val2);

    // [5] Status Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaStatus);
    printf("  [5] Status : 0x%x, ", regs_val1);

    // [6] Operation Mode Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaControl);
    printf("  [6] Control : 0x%x\n", regs_val1);

    // [7] Interrupt Enable Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaInterrupt);
    printf("  [7] Interrupt : 0x%x, ", regs_val1);

    // [8] Miss
    regs_val1 = read_register(DMA_BASE_ADDR, DmaMissedFr);
    printf("  [8] Missed : 0x%x\n", regs_val1);

    // [18-19] Current Host TX/RX Description Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaTxCurrDesc);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaRxCurrDesc);
    printf("  [18-19] Current Host TX/RX Description : 0x%x/0x%x\n", regs_val1, regs_val2);

    // [20-21] Current Host TX/RX Description Register
    regs_val1 = read_register(DMA_BASE_ADDR, DmaTxCurrAddr);
    regs_val2 = read_register(DMA_BASE_ADDR, DmaRxCurrAddr);
    printf("  [20-21] Current Host TX/RX Buffer Address : 0x%x/0x%x\n", regs_val1, regs_val2);
}

/* print DMA regs */
void printf_mac_regs()
{
    printf(">>[MAC Register]\n");
    uint32_t regs_val1, regs_val2;
    uint8_t mac_addr[6];

    // [0] MAC Configure Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacConfig);
    printf("  [0] Configure : 0x%x, ", regs_val1);

    // [1] MAC Frame Filter
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacFrameFilter);
    printf("  [1] Frame Filter : 0x%x\n", regs_val1);

    // [2-3] Hash Table High/Low Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacHashHigh);
    regs_val2 = read_register(GMAC_BASE_ADDR, GmacHashLow);
    printf("  [2-3] Hash Table High/Low : 0x%x-0x%x\n", regs_val1, regs_val2);

    // [6] Flow Control Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacFlowControl);
    printf("  [6] Flow Control : 0x%x, ", regs_val1);

    // [8] Version Register
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacVersion);
    printf("  [8] Version : 0x%x\n", regs_val1);

    // [14] Interrupt Status Register and Interrupt Mask
    regs_val1 = read_register(GMAC_BASE_ADDR, GmacInterruptStatus);
    regs_val2 = read_register(GMAC_BASE_ADDR, GmacInterruptMask);
    printf("  [14-15] Interrupt Status/Mask : 0x%x/0x%x\n", regs_val1, regs_val2);

    // MAC address
    gmac_get_mac_addr(mac_addr);
    printf("  [16-17] Mac Addr : %X:%X:%X:%X:%X:%X\n", mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);
}
#endif

void print_tx_dscrb(mac_t *mac)
{
    uint32_t i;
    printf("send buffer mac->saddr=0x%x ", mac->saddr);
    printf("mac->saddr_phy=0x%x ", mac->saddr_phy);
    printf("send discrb mac->td_phy=0x%x\n", mac->td_phy);
#if 0
    desc_t *send = (desc_t *)mac->td;
    for (i = 0; i < 1; i++)
    {
        printf("send[%d].tdes0=0x%x ", i, send[i].tdes0);
        printf("send[%d].tdes1=0x%x ", i, send[i].tdes1);
        printf("send[%d].tdes2=0x%x ", i, send[i].tdes2);
        printf("send[%d].tdes3=0x%x ", i, send[i].tdes3);
    }
#endif
}

void print_rx_dscrb(mac_t *mac)
{
    uint32_t i;
    printf("recieve buffer add mac->daddr=0x%x ", mac->daddr);
    printf("mac->daddr_phy=0x%x ", mac->daddr_phy);
    printf("recieve discrb add mac->rd_phy=0x%x\n", mac->rd_phy);
    desc_t *recieve = (desc_t *)mac->rd;
#if 0
    for(i=0;i<mac->pnum;i++)
    {
        printf("recieve[%d].tdes0=0x%x ",i,recieve[i].tdes0);
        printf("recieve[%d].tdes1=0x%x ",i,recieve[i].tdes1);
        printf("recieve[%d].tdes2=0x%x ",i,recieve[i].tdes2);
        printf("recieve[%d].tdes3=0x%x\n",i,recieve[i].tdes3);
    }
#endif
}

static uint32_t printf_recv_buffer(uint64_t recv_buffer)
{
    uint32_t i, flag, n;
    flag = 0;
    n = 0;
    for (i = 0; i < PSIZE * PNUM; i++)
    {
        if ((*((uint32_t *)recv_buffer + i) != 0) && (*((uint32_t *)recv_buffer + i) != 0xf0f0f0f0) && (*((uint32_t *)recv_buffer + i) != 0xf0f0f0f))
        {
            if (*((uint32_t *)recv_buffer + i) != 0xffffff)
            {
                printf(" %x ", *((uint32_t *)recv_buffer + i));

                if (n % 10 == 0 && n != 0)
                {
                    printf(" \n");
                }
                n++;
                flag = 1;
            }
        }
    }
    return flag;
}

static uint32_t kprintf_recv_buffer(uint64_t recv_buffer)
{
    uint32_t i, flag, n;
    flag = 0;
    n = 0;
    for (i = 0; i < PSIZE * PNUM; i++)
    {
        if ((*((uint32_t *)recv_buffer + i) != 0) && (*((uint32_t *)recv_buffer + i) != 0xf0f0f0f0) && (*((uint32_t *)recv_buffer + i) != 0xf0f0f0f))
        {
            if (*((uint32_t *)recv_buffer + i) != 0xffffff)
            {
                kprintf(" %x ", *((uint32_t *)recv_buffer + i));

                if (n % 10 == 0 && n != 0)
                {
                    kprintf(" \n");
                }
                n++;
                flag = 1;
            }
        }
    }
    return flag;
}

static uint32_t printk_recv_buffer(uint64_t recv_buffer)
{
    uint32_t i, flag, n;
    flag = 0;
    n = 0;
    for (i = 0; i < PSIZE * PNUM; i++)
    {
        if ((*((uint32_t *)recv_buffer + i) != 0) && (*((uint32_t *)recv_buffer + i) != 0xf0f0f0f0) && (*((uint32_t *)recv_buffer + i) != 0xf0f0f0f))
        {
            if (*((uint32_t *)recv_buffer + i) != 0xffffff)
            {
                printk(" %x ", *((uint32_t *)recv_buffer + i));

                if (n % 10 == 0 && n != 0)
                {
                    printk(" \n");
                }
                n++;
                flag = 1;
            }
        }
    }
    return flag;
}

/**
 * Clears all the pending interrupts.
 * If the Dma status register is read then all the interrupts gets cleared
 * @param[in] pointer to synopGMACdevice.
 * \return returns void.
 */
void clear_interrupt()
{
    uint32_t data;
    data = reg_read_32(DMA_BASE_ADDR + DmaStatus);
    reg_write_32(DMA_BASE_ADDR + DmaStatus, data & 0x1ffff);
}

void mac_irq_handle(void)
{
    volatile uint32_t * intenset_0 = (void *)0xffffffffbfe11428;
    volatile uint32_t * intenclr_0 = (void *)0xffffffffbfe1142c;
    static uint32_t num_package = 0;
    if (num_package == PNUM) {
        num_package = 0;
    }
    while (!(0x80000000 & rx_descriptor[num_package % NUM_DMA_DESC].tdes0) && num_package < PNUM){
        recv_flag[num_package] = 1;
        if (!queue_is_empty(&recv_block_queue)) {
            queue_push(&ready_queue, queue_dequeue(&recv_block_queue));
        }
        num_package++;
    }
    *intenclr_0 = (*intenclr_0) | (1 << 12);
    *intenset_0 = (*intenset_0) | (1 << 12);
    clear_interrupt();
}

void irq_enable(int IRQn)
{
    if (IRQn >= 0 && IRQn < 8) {
        set_cp0_status(get_cp0_status() | (1 << (8 + IRQn)));
    }
}

void mac_recv_handle(mac_t *mac)
{
    int i, j;
    do_wait_recv_package();
    for(i = 0; i < mac->pnum; i++){
        j = (i + 1) * PSIZE * 4;
        while(!recv_buf[--j] && j >= i * PSIZE * 4);
        buf_length[i] = j - i * PSIZE * 4 + 1;
    }
    i--;
    kprintf("the last rx_descriptor[%d].tdes0=0x%x\n", i, rx_descriptor[i].tdes0);
}

void set_sram_ctr()
{
    *((volatile uint32_t *)0xffffffffbfe10420) = 1; /* 使能GMAC0 DMA一致性 */
}

void disable_interrupt_all(mac_t *mac)
{
    reg_write_32(mac->dma_addr + DmaInterrupt, DmaIntDisable);
    return;
}

static void mac_recv_desc_init(mac_t *mac)
{
    uint32_t OWN = 0;
    uint32_t LS = 1, FS = 1;
    uint32_t DIC = 0;
    int i;
    int num_desc = (mac->pnum > NUM_DMA_DESC) ? NUM_DMA_DESC : mac->pnum;
    for(i = 0; i < num_desc; i++) {
        rx_descriptor[i].tdes0 = OWN << 31 | FS << 9 | LS << 8 | 0;
        rx_descriptor[i].tdes1 = DIC << 31 | (i == num_desc - 1) << 15 | 1 << 14 | mac->psize;
        rx_descriptor[i].tdes2 = mac->daddr_phy + i * mac->psize;
        rx_descriptor[i].tdes3 = mac->rd_phy + (((i + 1) % num_desc) * sizeof(desc_t));
    }
}

static void mii_dul_force(mac_t *mac)
{
    reg_write_32(mac->dma_addr, 0x80);

    uint32_t conf = 0xc800;

    reg_write_32(mac->mac_addr, reg_read_32(mac->mac_addr) | (conf) | (1 << 8));
    //enable recieve all

    reg_write_32(mac->mac_addr + 0x4, 0x80000001);
}

void dma_control_init(mac_t *mac, uint32_t init_value)
{
    reg_write_32(mac->dma_addr + DmaControl, init_value);
    return;
}
void set_mac_addr(mac_t *mac, uint8_t *MacAddr)
{
    uint32_t data;

    uint32_t MacHigh = 0x40, MacLow = 0x44;
    data = (MacAddr[5] << 8) | MacAddr[4];
    reg_write_32(mac->mac_addr + MacHigh, data);
    data = (MacAddr[3] << 24) | (MacAddr[2] << 16) | (MacAddr[1] << 8) | MacAddr[0];
    reg_write_32(mac->mac_addr + MacLow, data);
}

static void mac_send_desc_init(mac_t *mac)
{
    uint32_t OWN = 0;
    uint32_t IC = 1;
    uint32_t LS = 1, FS = 1;
    uint32_t DC = 1, CIC = 0;
    int i;
    int num_desc = (mac->pnum > NUM_DMA_DESC) ? NUM_DMA_DESC : mac->pnum;
    for (i = 0; i < num_desc; i++) {
        tx_descriptor[i].tdes0 = OWN << 31 | IC << 30 | LS << 29 | FS << 28 | DC << 27 | CIC << 26 | (i == num_desc - 1) << 21 | 1 << 20;
        tx_descriptor[i].tdes1 = mac->psize;
        tx_descriptor[i].tdes2 = mac->saddr_phy;// + i * mac->psize;
        tx_descriptor[i].tdes3 = mac->td_phy + (((i + 1) % num_desc) * sizeof(desc_t));
    }
}

/* buf_addr is the total recv buffer's address; size means the total size of recv buffer;
the total recv buffer may have some little recv buffer;
num means recv buffer's num ;length is each small recv buffer's size*/
uint32_t do_net_recv(uint64_t buf_addr, uint64_t size, uint64_t num)//, uint64_t length)
{

    mac_t mac;

    mac.mac_addr = GMAC_BASE_ADDR;
    mac.dma_addr = DMA_BASE_ADDR;

    mac.psize = PSIZE * 4; // 128bytes
    mac.pnum = num;       // pnum
    // mac.daddr = buf_addr;
    mac.daddr = (uint64_t)&recv_buf;
    mac.daddr_phy = (uint64_t)(&recv_buf) & 0x1fffffff;
    mac.rd = (uint64_t)&rx_descriptor;
    mac.rd_phy = (uint64_t)(&rx_descriptor) & 0x1fffffff;

    mac_recv_desc_init(&mac);
    dma_control_init(&mac, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
    clear_interrupt(&mac);

    mii_dul_force(&mac);

    reg_write_32(GMAC_BASE_ADDR, (1 << 8) | 0x4);
    reg_write_32(DMA_BASE_ADDR + DmaRxBaseAddr, (uint32_t)mac.rd_phy);
    reg_write_32(DMA_BASE_ADDR + 0x18, reg_read_32(DMA_BASE_ADDR + 0x18) | 0x02200002); // start tx, rx
    reg_write_32(DMA_BASE_ADDR + 0x1c, 0x10001 | (1 << 6));

    reg_write_32(DMA_BASE_ADDR + 0x1c, DMA_INTR_DEFAULT_MASK);

    /*  YOU NEED ADD RECV CODE*/
    // do_wait_recv_package();
    int i;
    int OWN = 1;
    for (i = 0; i < mac.pnum; i++) {
        rx_descriptor[i].tdes0 |= OWN << 31;
        reg_write_32(DMA_BASE_ADDR + DmaRxPollDemand, 0x00000001);
    }
    mac_recv_handle(&mac);
    // for (i = 0; i < mac.pnum; i++) {
    //     while(0x80000000 & rx_descriptor[i].tdes0);
    // }
    // i--;
    // printf("the last rx_descriptor[%d].tdes0 = 0x%x\n", i, rx_descriptor[i].tdes0);
    memcpy(buf_addr, &recv_buf, size * 4);
    kprintf_recv_buffer(buf_addr);
    return 0;
}

/* buf_addr is sended buffer's address; size means sended buffer's size;
num means sended buffer's times*/
void do_net_send(uint64_t buf_addr, uint64_t size, uint64_t num)
{
    uint64_t td_phy;
    mac_t mac;
    mac.mac_addr = GMAC_BASE_ADDR;
    mac.dma_addr = DMA_BASE_ADDR;

    memcpy(&send_buf, buf_addr, size * 4); 

    mac.psize = size * 4;
    mac.pnum = num;
    // td_phy = mac.td_phy;
    mac.saddr = (uint64_t)&send_buf;
    mac.saddr_phy = (uint64_t)(&send_buf) & 0x1fffffff;
    mac.td = (uint64_t)&tx_descriptor;
    mac.td_phy = (uint64_t)(&tx_descriptor) & 0x1fffffff;

    mac_send_desc_init(&mac);
    dma_control_init(&mac, DmaStoreAndForward | DmaTxSecondFrame | DmaRxThreshCtrl128);
    clear_interrupt(&mac);

    mii_dul_force(&mac);

    reg_write_32(GMAC_BASE_ADDR, (1 << 8) | 0x8);                    // enable MAC-TX
    // reg_write_32(GMAC_BASE_ADDR, reg_read_32(GMAC_BASE_ADDR) | 0x8);                    // enable MAC-TX
    reg_write_32(DMA_BASE_ADDR + DmaTxBaseAddr, (uint32_t)mac.td_phy);
    reg_write_32(DMA_BASE_ADDR + 0x18, reg_read_32(DMA_BASE_ADDR + 0x18) | 0x02202000); //0x02202002); // start tx, rx
    reg_write_32(DMA_BASE_ADDR + 0x1c, 0x10001 | (1 << 6));
    reg_write_32(DMA_BASE_ADDR + 0x1c, DMA_INTR_DEFAULT_MASK);

    /*  YOU NEED ADD SEND CODE*/
    int i;
    int OWN = 1;
    for (i = 0; i < mac.pnum; i++) {
        tx_descriptor[i].tdes0 |= OWN << 31;
        reg_write_32(DMA_BASE_ADDR + DmaTxPollDemand, 0x00000001);
    }
    for (i = 0; i < mac.pnum; i++) {
        while(0x80000000 & tx_descriptor[i].tdes0);
    }
}
void set_mac_int()
{
    volatile uint8_t *entry_gmac0;
    entry_gmac0 = 0xffffffff1fe1140c | 0xa0000000;
    *entry_gmac0 = 0x41; //0 core ip6 int4
}
void do_init_mac(void)
{
    mac_t test_mac;
    uint32_t i;

    test_mac.mac_addr = GMAC_BASE_ADDR;
    test_mac.dma_addr = DMA_BASE_ADDR;

    test_mac.psize = PSIZE * 4; // 64bytes
    test_mac.pnum = PNUM;       // pnum
    uint8_t mac_addr[6];

    // gmac_get_mac_addr(mac_addr);
    set_sram_ctr(); /* 使能GMAC0 */
                    //   s_reset(&test_mac); //will interrupt
    disable_interrupt_all(&test_mac);
    // set_mac_addr(&test_mac, mac_addr);
    register_irq_handler(12, mac_irq_handle);
    reg_write_32(GMAC_BASE_ADDR + 0X3C, 1);
    set_mac_int();
}

void do_wait_recv_package(void)
{
    int i;
    bzero(recv_flag, 4 * PNUM);
    for (i = 0; i < PNUM; i++) {
        if (recv_flag[i] == 0) {
            do_block(&recv_block_queue);
        }
    }
}
