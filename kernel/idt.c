#include<interrupts.h>
#include<printf.h>
#include<io.h>
#include<section.h>
#include<gdt.h>

#define IDT_ENTRY   256

unsigned long long _idt[IDT_ENTRY] BOOT_DATA;
unsigned short _idt_limit=sizeof(_idt)-1;

void _set_idt_entry(unsigned int vector,unsigned  short seg_selector,void (*isr)() ,unsigned char dpl)
{
     unsigned int offset=(unsigned int)isr;
    _idt[vector]=((offset & 0xffff0000)| IDT_ATTR(dpl));
    _idt[vector]<<=32;
    _idt[vector] |=(seg_selector<<16|(offset&0x0000ffff));
    
}
void do_peserved_0()
{
    printf("aaa\n");
}

void do_about_double()
{
    printf("double_fault\n");
}


void init_idt()
{
    _set_idt_entry(FAULT_DIVISION_ERROR,SEG_KERNEL_CODE,division_error,0);
    _set_idt_entry(ABORT_DOUBLE_FAULT,SEG_KERNEL_CODE,about_double,0);
    _set_idt_entry(FAULT_PESERVED_0,SEG_KERNEL_CODE,peserved_0,0);
    _set_idt_entry(0x20,SEG_KERNEL_CODE,timer_isr,0);
    _set_idt_entry(0x21,SEG_KERNEL_CODE,keyboard_isr,0);
    _set_idt_entry(FAULT_GRNERAL_PROTECTION,SEG_KERNEL_CODE,general_protection,0);
    _set_idt_entry(FAULT_PAGE_FAULT,SEG_KERNEL_CODE,page_fault,0);
    _set_idt_entry(0x80,SEG_KERNEL_CODE,system_call,3);
}