#include<printf.h>
#include<process.h>
#include<stdint.h>

void do_general_protection(uint32_t error_code)
{
    printf("general_protection:%d\n",error_code);
    while(1);
}