void _kmain() {
    char attr = 0x07;
    unsigned short * vga_buf = (unsigned short *) 0xB8000;

    char msg[] = "Hello from the kernel! ";
    for (unsigned int i = 0; msg[i]; i++) {
        vga_buf[0] = msg[i] | attr << 8;
        vga_buf++;
    }

    while (1);
}
