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
    while(1);
}

void do_about_double()
{
    printf("double_fault\n");
    while(1);
}

void do_int1()
{
    printf(" [1] \n");
    while(1);
}

void do_int2()
{
    printf(" [2] \n");
    while(1);
}

void do_int3()
{
    printf(" [3] \n");
    while(1);
}

void do_int4()
{
    printf(" [4] \n");
    while(1);
}

void do_int5()
{
    printf(" [5] \n");
    while(1);
}

void do_int6()
{
    printf(" [6] \n");
    while(1);
}

void do_int7()
{
    printf(" [7] \n");
    while(1);
}

void do_intA()
{
    printf(" [A] \n");
    while(1);
}

void do_intB()
{
    printf(" [B] \n");
    while(1);
}

void do_intC()
{
    printf(" [C] \n");
    while(1);
}

void do_intF()
{
    printf(" [F] \n");
    while(1);
}

void do_int10()
{
    printf(" [10] \n");
    while(1);
}

void do_int11()
{
    printf(" [11] \n");
    while(1);
}

void do_int12()
{
    printf(" [12] \n");
    while(1);
}

void do_int13()
{
    printf(" [13] \n");
    while(1);
}

void do_int14()
{
    printf(" [14] \n");
    while(1);
}

void do_int15()
{
    printf(" [15] \n");
    while(1);
}

uint32_t cli(void) {
    uint32_t eflags;

    __asm__ __volatile__(
        "pushfl\n\t"          // 将 EFLAGS 压栈
        "popl %0\n\t"         // 弹出到 eflags 变量（保存原状态）
        "cli\n\t"             // 执行关中断指令
        : "=r"(eflags)        // 输出：eflags 存储操作前的 EFLAGS
        : "0"(eflags)         // 输入：复用输出寄存器（仅占位）
        : "memory"            // 告诉编译器内存可能被修改，禁止优化
    );

    return eflags;
}

uint32_t sti(void) {
    uint32_t eflags;

    __asm__ __volatile__(
        "pushfl\n\t"          // 将 EFLAGS 压栈
        "popl %0\n\t"         // 弹出到 eflags 变量（保存原状态）
        "sti\n\t"             // 执行开中断指令
        : "=r"(eflags)        // 输出：eflags 存储操作前的 EFLAGS
        : "0"(eflags)         // 输入：复用输出寄存器
        : "memory"            // 禁止编译器优化
    );

    return eflags;
}



void init_idt()
{
    _set_idt_entry(FAULT_DIVISION_ERROR,SEG_KERNEL_CODE,division_error,0);
    _set_idt_entry(FAULT_TRAP_DEBUG_EXCEPTION,SEG_KERNEL_CODE,int_1,0);
    _set_idt_entry(INT_NMI,SEG_KERNEL_CODE,int_2,0);
    _set_idt_entry(TRAP_BREAKPOINT,SEG_KERNEL_CODE,int_3,0);
    _set_idt_entry(TRAP_OVERFLOW,SEG_KERNEL_CODE,int_4,0);
    _set_idt_entry(FAULT_BOUND_EXCEED,SEG_KERNEL_CODE,int_5,0);
    _set_idt_entry(FAULT_INVALID_OPCODE,SEG_KERNEL_CODE,int_6,0);
    _set_idt_entry(FAULT_NO_MATH_PROCESSOR,SEG_KERNEL_CODE,int_7,0);  
    _set_idt_entry(ABORT_DOUBLE_FAULT,SEG_KERNEL_CODE,about_double,0);
    _set_idt_entry(FAULT_PESERVED_0,SEG_KERNEL_CODE,peserved_0,0);
    _set_idt_entry(FAULT_INVALID_TSS,SEG_KERNEL_CODE,int_A,0);
    _set_idt_entry(FAULT_SEG_NOT_PRESENT,SEG_KERNEL_CODE,int_B,0);
    _set_idt_entry(FAULT_STACK_SEG_FAULT,SEG_KERNEL_CODE,int_C,0);
    _set_idt_entry(FAULT_GRNERAL_PROTECTION,SEG_KERNEL_CODE,general_protection,0);
    _set_idt_entry(FAULT_PAGE_FAULT,SEG_KERNEL_CODE,page_fault,0);
    _set_idt_entry(FAULT_RESERVED_1,SEG_KERNEL_CODE,int_F,0);
    _set_idt_entry(FAULT_X87_FAULT ,SEG_KERNEL_CODE,int_10,0);
    _set_idt_entry(FULT_ALIGNMENT_CHECK,SEG_KERNEL_CODE,int_11,0);
    _set_idt_entry(ABORT_MACHINE_CHECK ,SEG_KERNEL_CODE,int_12,0);
    _set_idt_entry(FAULT_SIMD_FP_EXCEPTION ,SEG_KERNEL_CODE,int_13,0);
    _set_idt_entry(FAULT_VIRTUALIZATION_EXCEPTION,SEG_KERNEL_CODE,int_14,0);
    _set_idt_entry(FAULT_CONTROL_PROTECTION ,SEG_KERNEL_CODE,int_15,0);
    _set_idt_entry(0x20,SEG_KERNEL_CODE,context_switch,0);
    _set_idt_entry(0x21,SEG_KERNEL_CODE,keyboard_isr,0);
    _set_idt_entry(0x80,SEG_KERNEL_CODE,system_call,3);

    // sti();
}