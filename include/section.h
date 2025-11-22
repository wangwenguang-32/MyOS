#ifndef  SECCION_H
#define SECCION_H

#ifndef ASSEMBLY

#define TASK0_TEXT  __attribute__((section(".task0.text")))
#define TASK0_DATA  __attribute__((section(".task0.data")))

#else

#define TASK0_TEXT .section .task0.text
#define TASK0_DATA .section .task0.data

#endif

#define BOOT_DATA __attribute__((section(".boot.data")))

#define DATA __attribute__((section(".data")))
#define DATA_MEM_PAGE __attribute__((section(".data.mem_page")))


#endif