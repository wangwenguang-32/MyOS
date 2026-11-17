#ifndef COMMON_H
#define COMMON_H

#define loop(n)    int i; for(i=0; i<(n); i++)

#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define ABS(x)    ((x) < 0 ? -(x) : (x))

// 数组元素个数
#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

#define bool  uint8_t
#define true  1
#define false 0

#define offsetof(type,member)   ((uint32_t)(&(((type*)0)->member)))


static void memcpy(uint8_t*dst,uint8_t*src,uint32_t size)
{
    loop(size)
    {
        *(dst+i)=*(src+i);
    }
}

static void memzero(uint8_t*src,uint32_t size)
{
    loop(size)
    {
        *(src+i)=0;
    }
}

#endif