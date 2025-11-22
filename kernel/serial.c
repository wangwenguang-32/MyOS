#include<io.h>
#include<stdint.h>

#define SERIAL_COM1_BASE 0x3F8

// 串口寄存器偏移（基于COM1基地址）
#define SERIAL_DATA_REG(port)          (port)      // 数据寄存器，偏移0
#define SERIAL_IER_REG(port)           (port + 1)  // 中断允许寄存器，偏移1
#define SERIAL_FCR_REG(port)           (port + 2)  // FIFO控制寄存器，偏移2
#define SERIAL_LCR_REG(port)           (port + 3)  // 线路控制寄存器，偏移3
#define SERIAL_MCR_REG(port)           (port + 4)  // 调制解调器控制寄存器，偏移4
#define SERIAL_LSR_REG(port)           (port + 5)  // 线路状态寄存器，偏移5
#define SERIAL_MSR_REG(port)           (port + 6)  // 调制解调器状态寄存器，偏移6
#define SERIAL_SCR_REG(port)           (port + 7)  // 暂存寄存器，偏移7

// 线路状态寄存器(LSR)的位定义
#define SERIAL_LSR_THRE 0x20  // 第5位：发送保持寄存器空（THRE）
#define SERIAL_LSR_DR   0x01  // 第0位：数据就绪（DR，接收数据时用）


/**
 * @brief 等待串口发送缓冲区为空
 * @param com 串口基地址（如SERIAL_COM1_BASE）
 * @return 始终返回0（成功）
 */
static inline int serial_wait_transmit_empty(uint16_t com) {
    // 循环读取线路状态寄存器，直到THRE位为1（发送缓冲区空）
    while ((inb(SERIAL_LSR_REG(com)) & SERIAL_LSR_THRE) == 0);
    return 0;
}

/**
 * @brief 初始化串口（配置为9600波特率、8N1）
 * @param com 串口基地址（如SERIAL_COM1_BASE）
 */
void serial_init(uint16_t com) {
    // 1. 禁用中断
    outb(SERIAL_IER_REG(com), 0x00);

    // 2. 设置波特率：先置位LCR的DLAB位（第7位），解锁波特率除数寄存器
    outb(SERIAL_LCR_REG(com), 0x80);

    // 3. 写入波特率除数（9600波特率对应除数0x000C，高位+低位）
    outb(SERIAL_DATA_REG(com), 0x0C);  // 除数低位（LSB）
    outb(SERIAL_IER_REG(com), 0x00);   // 除数高位（MSB）

    // 4. 配置线路格式：8位数据位、1位停止位、无校验，同时清除DLAB位
    outb(SERIAL_LCR_REG(com), 0x03);

    // 5. 配置FIFO：启用FIFO、清空接收/发送FIFO、FIFO触发级别14字节
    outb(SERIAL_FCR_REG(com), 0xC7);

    // 6. 配置调制解调器：启用DTR和RTS（硬件流控，可选）
    outb(SERIAL_MCR_REG(com), 0x0B);

    // 7. 再次启用中断（可选，若不需要中断则保持0x00）
    outb(SERIAL_IER_REG(com), 0x01);
}

/**
 * @brief 向串口发送一个字符
 * @param com  串口基地址（如SERIAL_COM1_BASE）
 * @param c    要发送的字符（支持ASCII可见字符、换行\n、回车\r等）
 */
void serial_putchar(uint16_t com, char c) {
    // 等待发送缓冲区为空
    serial_wait_transmit_empty(com);

    // 对换行符\n做特殊处理：自动补充回车\r，避免串口输出换行错位
    if (c == '\n') {
        outb(SERIAL_DATA_REG(com), '\r');
        serial_wait_transmit_empty(com);
    }

    // 向数据寄存器写入字符，完成发送
    outb(SERIAL_DATA_REG(com), (uint8_t)c);
}

/**
 * @brief 向串口发送一个字符串（带结束符\0）
 * @param com   串口基地址（如SERIAL_COM1_BASE）
 * @param str   要发送的字符串
 */
void serial_puts(uint16_t com, const char* str) {
    if (str == 0) return;
    while (*str != '\0') {
        serial_putchar(com, *str);
        str++;
    }
}

