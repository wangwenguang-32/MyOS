#include<stdint.h>
#include<task0.h>
#include<section.h>

void TASK0_TEXT task0_main()
{

}

int number1=50;
int number2=50;

void TASK0_TEXT print_hello1()
{
    if(number1>0)
    {
        printf("Hello   ");
        number1--;
    }
    int i=0;
    for(i;i<0x100000;i++);
    
}

void TASK0_TEXT print_hello2()
{
     if(number2>0)
    {
        printf("World   ");
        number2--;
    }
     int i=0;
    for(i;i<0x100000;i++);
}