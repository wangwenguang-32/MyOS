#ifndef TASK0_H
#define TASK0_H

#ifndef ASSEMBLY

#define TASK0_TEXT  __attribute__((section(".task0.text")))
#define TASK0_DATA  __attribute__((section(".task0.data")))

#else

#define TASK0_TEXT .section .task0.text
#define TASK0_DATA .section .task0.data


#endif


#endif