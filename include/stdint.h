#ifndef  STDINT_H
#define STDINT_H

// 标准整数类型定义

// 8位整数类型
typedef signed char int8_t;
typedef unsigned char uint8_t;

// 16位整数类型
typedef short int16_t;
typedef unsigned short uint16_t;

// 32位整数类型
typedef int int32_t;
typedef unsigned int uint32_t;

// 64位整数类型
#ifdef _MSC_VER
    typedef __int64 int64_t;
    typedef unsigned __int64 uint64_t;
#else
    typedef long long int64_t;
    typedef unsigned long long uint64_t;
#endif

// 指针大小的整数类型
#ifdef _WIN64
    typedef long long intptr_t;
    typedef unsigned long long uintptr_t;
#else
    typedef int intptr_t;
    typedef unsigned int uintptr_t;
#endif

// 最大整数类型
typedef int64_t intmax_t;
typedef uint64_t uintmax_t;

// 最小和最大值宏
#define INT8_MIN    (-128)
#define INT8_MAX    127
#define UINT8_MAX   255

#define INT16_MIN   (-32768)
#define INT16_MAX   32767
#define UINT16_MAX  65535

#define INT32_MIN   (-2147483647 - 1)
#define INT32_MAX   2147483647
#define UINT32_MAX  4294967295U

#define INT64_MIN   (-9223372036854775807LL - 1)
#define INT64_MAX   9223372036854775807LL
#define UINT64_MAX  18446744073709551615ULL

#endif // STDINT_H

