#include<tty.h>
#include<printf.h>
#include<stdint.h>
#include<io.h>
#include<vga.h>

// 设置光标位置
void cursor_set_pos(uint8_t row, uint8_t col) {
    // 1. 计算光标位置（行*80 + 列）
    uint16_t pos = (uint16_t)row * COLUMNS + (uint16_t)col;
    
    // 2. 写入低8位到寄存器0x0F
    outb(0x3D4, CURSOR_POS_LOW);    // 指定要操作的寄存器
    outb(0x3D5, (uint8_t)(pos & 0xFF));  // 写入低8位
    
    // 3. 写入高8位到寄存器0x0E
    outb(0x3D4, CURSOR_POS_HIGH);   // 指定要操作的寄存器
    outb(0x3D5, (uint8_t)(pos >> 8));    // 写入高8位
}

// 获取当前光标位置
void cursor_get_pos(uint8_t *row, uint8_t *col) {
    uint16_t pos = 0;
    
    // 读低8位
    outb(0x3D4, CURSOR_POS_LOW);
    pos |= inb(0x3D5);
    
    // 读高8位
    outb(0x3D4, CURSOR_POS_HIGH);
    pos |= (uint16_t)inb(0x3D5) << 8;
    
    // 转换为行、列
    *row = pos / COLUMNS;
    *col = pos % COLUMNS;
}




/**
 * @brief 往控制台输出单个字符（支持换行、退格等基础控制符）
 * @param c 要输出的字符（ASCII码）
 */
void console_putchar(char c) {
    
    // 处理控制字符
    switch (c) {
        // 换行：列归0，行+1（超出则滚屏）
        case '\n':
            putchar(c);
            break;
        
        // 回车：仅列归0
        case '\r':
            putchar(c);
            break;
        
        // 退格：列-1（不能小于0）
        case '\b':
            if (xpos > 0) {
                xpos--;
            }
            break;
        
        // 普通可打印字符（ASCII 32~126）
        default:
            putchar(c);
            break;
    }

    cursor_set_pos(ypos,xpos);
    // cursor_set_visibility(1);
    
}


void write(tty_t* tty)
{
    char c;
    while (!EMPTY(&tty->write))
    {
        GETCH_HEAD(&tty->write,&c);
        console_putchar(c);
    }
    
    
}






