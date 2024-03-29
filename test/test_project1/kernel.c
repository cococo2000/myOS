void __attribute__((section(".entry_function"))) _start(void)
{
    // print "Hello OS!"
    void (*printstr)(char *) = (void *)0xffffffff8f0d5534;
    (*printstr)("\rHello OS!\n");

    int i = 0;
    while(i < 1024){
        (*printstr)("Hello World!\t");
        i++;
    }

    // echo the input char
    (*printstr)("\r\nReady for input:");
    void (*printchar)(char) = (void *)0xffffffff8f0d5570;
    volatile char * state = (char *)0xffffffffbfe00005;     // serial port state register
    char * input = (char *)0xffffffffbfe00000;
    while(1){
        if((*state) & 0x01){
            (*printchar)(*input);
        }
    }
}
