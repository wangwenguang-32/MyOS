#ifndef STRING_H
#define STRING_H

#include<stdint.h>

static void memcpy(uint8_t*dst,uint8_t*src,uint32_t size)
{
    int i=0;
    for(i;i<size;i++)
    {
        *(dst+i)=*(src+i);
    }
}




#endif