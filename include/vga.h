#ifndef VGA_H
#define VGA_H


#define COLUMNS                 80
#define LINES                   25
#define ATTRIBUTE               7
#define VIDEO                   0xC00B8000

#include<tty.h>

extern uint32_t xpos;
extern uint32_t ypos;
extern volatile unsigned char *video;


#define CURSOR_POS_LOW  0x0F  // 光标位置低8位
#define CURSOR_POS_HIGH 0x0E  // 光标位置高8位
#define CURSOR_CTRL     0x0A  // 光标控制（显隐/闪烁）

void vga_write(tty_t* tty);


#endif