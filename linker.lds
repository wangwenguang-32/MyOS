SECTIONS {
    . = 0x100000;
    _boot_start = .;
    .boot.text  : { *(.boot.text) }
    .boot.bss   : { *(.boot.bss) }
    .boot.data  : { *(.boot.data) }
    _boot_end = . ;

    . = ALIGN(4096);                 
    _task0_phys_start = .;             
    . = 0x8048000;                    
    .task0.text : AT(_task0_phys_start) {  
        *(.task0.text)
    }
    task0_phys_tmp = _task0_phys_start + SIZEOF(.task0.text);  

    . = task0_phys_tmp;    
    . = ALIGN(4096);     
    _task0_phys_end = .;      
    . += 0xC0000000;                  
     _kernel_start = . ;

    .text BLOCK(4K) : AT(ADDR(.text) - 0xC0000000) {  
        *(.text)
    }
    .rodata : { *(.rodata) }
    .data : { *(.data) }
    .bss : { *(.bss) }
    . = ALIGN(4096);
    _kernel_end = . ;
}