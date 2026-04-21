#ifndef VGA_H
#define VGA_H


/*  Some screen stuff. */
/*  The number of columns. */
#define COLUMNS                 80
/*  The number of lines. */
#define LINES                   24
/*  The attribute of an character. */
#define ATTRIBUTE               7
/*  The video memory address. */
#define VIDEO                   0xC00B8000

/*  Variables. */
/*  Save the X position. */
extern int xpos;
/*  Save the Y position. */
extern int ypos;
/*  Point to the video memory. */
extern volatile unsigned char *video;


// 光标寄存器编号
#define CURSOR_POS_LOW  0x0F  // 光标位置低8位
#define CURSOR_POS_HIGH 0x0E  // 光标位置高8位
#define CURSOR_CTRL     0x0A  // 光标控制（显隐/闪烁）


#endif