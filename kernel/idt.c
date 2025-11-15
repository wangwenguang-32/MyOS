#include<interrupts.h>
#include<printf.h>
#include<io.h>

#define IDT_ENTRY   200

unsigned long long _idt[IDT_ENTRY] __attribute__((section(".data")));
unsigned short _idt_limit=sizeof(_idt)-1;

void _set_idt_entry(unsigned int vector,unsigned  short seg_selector,void (*isr)() ,unsigned char dpl)
{
     unsigned int offset=(unsigned int)isr;
    _idt[vector]=((offset & 0xffff0000)| IDT_ATTR(dpl));
    _idt[vector]<<=32;
    _idt[vector] |=(seg_selector<<16|(offset&0x0000ffff));
    
}



void _init_idt()
{
    _set_idt_entry(FAULT_DIVISION_ERROR,0x08,division_error,0);
    _set_idt_entry(0x20,0x08,timer_isr,0);
    _set_idt_entry(0x21,0x08,keyboard_isr,0);
    _set_idt_entry(FAULT_GRNERAL_PROTECTION,0x08,general_protection,0);
    _set_idt_entry(FAULT_PAGE_FAULT,0x08,page_fault,0);
    _set_idt_entry(0x80,0x08,system_call,3);
}