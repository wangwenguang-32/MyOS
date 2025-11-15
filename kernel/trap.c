#include<printf.h>
#include<process.h>
#include<stdint.h>
void do_division_error()
{
        printf("aaa\n");
}



void do_general_protection(uint32_t error_code)
{
    printf("general_protection:%d\n",error_code);
    while(1);
}

void do_page_fault(uint32_t error_code)
{
    printf("page fault:%d\n",error_code);
    while(1);
}

/* 定时器中断处理函数 */
uint32_t timer_interrupt_handler(void)
{
    lapic_eoi();
    if ( current->time_slice > 0) {
        current->time_slice--;
        return 1;
    }
    return 0;
}
