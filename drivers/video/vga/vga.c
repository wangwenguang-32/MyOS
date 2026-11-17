#include<vga.h>
#include<stdint.h>
#include<io.h>
#include<tty.h>



uint32_t xpos=0;
uint32_t ypos=0;
volatile uint8_t *video = VIDEO;

void vga_scroll_up()
{
    uint16_t y=0;
    uint16_t*buffer=(uint16_t*)VIDEO;
    for(y;y<LINES-1;y++)
    {
        for(uint16_t x = 0;x<COLUMNS;x++)
        {
            
            buffer[x+y*COLUMNS]=buffer[x+(y+1)*COLUMNS];
        }
    }
    for(uint16_t x=0;x<COLUMNS;x++)
    {
        buffer[x+y*COLUMNS]=0x0720;
    }

}

void cursor_set_pos(uint8_t row, uint8_t col) 
{
    uint16_t pos = (uint16_t)row * COLUMNS + (uint16_t)col;
    
    outb(0x3D4, CURSOR_POS_LOW);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    
    outb(0x3D4, CURSOR_POS_HIGH);
    outb(0x3D5, (uint8_t)(pos >> 8));
}

void cursor_get_pos(uint8_t *row, uint8_t *col) 
{
    uint16_t pos = 0;
    
    outb(0x3D4, CURSOR_POS_LOW);
    pos |= inb(0x3D5);
    outb(0x3D4, CURSOR_POS_HIGH);
    pos |= (uint16_t)inb(0x3D5) << 8;
    
    *row = pos / COLUMNS;
    *col = pos % COLUMNS;
}

void cursor_enable() 
{
    uint8_t cursor_start = 0x0E;
    uint8_t cursor_end   = 0x0F;

    outb(0x3D4, 0x0A);
    outb(0x3D5, cursor_start & 0x1F);

    // 写入 CRTC 0x0B = 光标结束
    outb(0x3D4, 0x0B);
    outb(0x3D5, cursor_end);
    inb(0x3DA);  

    // 2. 打开闪烁
    uint8_t data = inb(0x3D8);  // 读
    data |= (1 << 5);          // 开启闪烁
    outb(0x3D8, data);         // 写回

}


void vga_write(tty_t* tty)
{
    uint8_t c;
    while (!EMPTY(&tty->write))
    {
        GETCH_HEAD(&tty->write,&c);
        if(c>=32&&c<=126)
        {
            *(video + (xpos+ ypos*COLUMNS)*2) = c&0xFF;
            *(video + (xpos+ ypos*COLUMNS)*2 + 1) = ATTRIBUTE;

            xpos++;
            if(xpos==COLUMNS)
            {
                xpos=0;
                if(ypos==LINES-1)
                {
                    vga_scroll_up();
                }
                else
                {
                    ypos++;
                }
            }
        }
        else
        {
            switch (c) 
            {
                case '\n':
                {
                    xpos=0;
                    if(ypos==LINES-1)
                    {
                        vga_scroll_up();
                    }
                    else
                    {
                        ypos++;
                    }
                    break;
                }
                
        
                case '\r':
                {
                    xpos=0;
                    break;
                }
        
                case '\b':
                {
                    if (xpos > 0) 
                    {
                        xpos--;
                    }
                    break;
                }
            }

        }
        cursor_set_pos(ypos,xpos);
    }
    
    
}