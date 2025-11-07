SECTIONS {
    . =0x100000;
    _boot_start = .;
    .boot.text  : {
        *(.boot.text)
    }
    .boot.bss : {
        *(.boot.bss)
    }
    .boot.data : {
        *(.boot.data)
    }
    _boot_end = . ;
    . += 0xC0000000;
    _kernel_start =  . ;
    .text BLOCK(4K) : AT(ADDR(.text)-0xC0000000){
        *(.text)
    }
    .rodata : {
        *(.rodata)
    }
    .data : {
        *(.data)
    }
    .bss : {
        *(.bss)
    }

    .bss.mem_page : {
        *(.bss.mem_page)
    }

    _kernel_end =  . ;

}