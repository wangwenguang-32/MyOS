#ifndef PID_H
#define PID_H

#include <stdint.h>

#define PID_MIN            1
#define PID_MAX            32768
#define PID_INVALID        (-1)

void pid_allocator_init(void);
int32_t pid_alloc(void);
void pid_free(int32_t pid);
int pid_is_in_use(int32_t pid);

#endif /* PID_H */

