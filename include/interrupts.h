#ifndef INTERRUPTS_H
#define INTERRUPTS_H
#define  IDT_ATTR(dpl)                              ((0x70<<5) | (dpl & 3)<<13 | 1 << 15)

#define FAULT_DIVISION_ERROR                                    0x0
#define FAULT_TRAP_DEBUG_EXCEPTION                        0x1
#define INT_NMI                                                            0x2
#define TRAP_BREAKPOINT                                             0x3
#define TRAP_OVERFLOW                                                0x4
#define FAULT_BOUND_EXCEED                                       0x5
#define FAULT_INVALID_OPCODE                                     0x6
#define FAULT_NO_MATH_PROCESSOR                            0x7
#define ABORT_DOUBLE_FAULT                                       0x8
#define FAULT_PESERVED_0                                            0x9
#define FAULT_INVALID_TSS                                            0xA
#define FAULT_SEG_NOT_PRESENT                                  0xB
#define FAULT_STACK_SEG_FAULT                                   0xC
#define FAULT_GRNERAL_PROTECTION                             0xD
#define FAULT_PAGE_FAULT                                             0xE
#define FAULT_RESERVED_1                                            0xF
#define FAULT_X87_FAULT                                               0x10
#define FULT_ALIGNMENT_CHECK                                    0x11
#define ABORT_MACHINE_CHECK                                     0x12
#define FAULT_SIMD_FP_EXCEPTION                                 0x13
#define FAULT_VIRTUALIZATION_EXCEPTION                      0x14
#define FAULT_CONTROL_PROTECTION                             0x15

void division_error();
void keyboard_isr();
void page_fault();
void general_protection();
void system_call();
void timer_isr();

#endif
