#include <vga.h>
#include <io.h>
#include <mm_lock.h>

#define CRTC_INDEX 0x3d4
#define CRTC_DATA  0x3d5
uint32_t xpos, ypos;
volatile uint8_t *video = (volatile uint8_t *)VIDEO;

static uint8_t crtc_read(uint8_t index)
{
    outb(CRTC_INDEX, index);
    return inb(CRTC_DATA);
}
static void crtc_write(uint8_t index, uint8_t value)
{
    outb(CRTC_INDEX, index);
    outb(CRTC_DATA, value);
}

void cursor_set_pos(uint8_t row, uint8_t col)
{
    uint32_t flags = mm_irq_save();
    uint16_t position;
    if (row >= LINES) row = LINES - 1;
    if (col >= COLUMNS) col = COLUMNS - 1;
    position = row * COLUMNS + col;
    crtc_write(CURSOR_POS_LOW, position & 255);
    crtc_write(CURSOR_POS_HIGH, position >> 8);
    mm_irq_restore(flags);
}

void cursor_get_pos(uint8_t *row, uint8_t *col)
{
    uint32_t flags = mm_irq_save();
    uint16_t position = crtc_read(CURSOR_POS_LOW);
    position |= (uint16_t)crtc_read(CURSOR_POS_HIGH) << 8;
    if (row) *row = position / COLUMNS;
    if (col) *col = position % COLUMNS;
    mm_irq_restore(flags);
}

void cursor_enable(void)
{
    uint32_t flags = mm_irq_save();
    uint8_t last_scanline = crtc_read(0x09) & 0x1f;
    uint8_t start = last_scanline ? last_scanline - 1 : 0;
    /* Clear cursor-disable bit; preserve unrelated CRTC control bits. */
    crtc_write(0x0a, (crtc_read(0x0a) & 0xc0) | start);
    crtc_write(0x0b, (crtc_read(0x0b) & 0xe0) | last_scanline);
    cursor_set_pos(ypos, xpos);
    mm_irq_restore(flags);
}

void vga_scroll_up(void)
{
    volatile uint16_t *cells = (volatile uint16_t *)VIDEO;
    uint32_t flags = mm_irq_save(), i;
    for (i = 0; i < (LINES - 1) * COLUMNS; i++) cells[i] = cells[i + COLUMNS];
    for (; i < LINES * COLUMNS; i++) cells[i] = (ATTRIBUTE << 8) | ' ';
    mm_irq_restore(flags);
}

void vga_clear(void)
{
    uint32_t flags = mm_irq_save(), i;
    volatile uint16_t *cells = (volatile uint16_t *)VIDEO;
    for (i = 0; i < LINES * COLUMNS; i++) cells[i] = (ATTRIBUTE << 8) | ' ';
    xpos = ypos = 0;
    cursor_enable();
    mm_irq_restore(flags);
}

void vga_putchar(uint8_t c)
{
    uint32_t flags = mm_irq_save();
    if (c == '\r') xpos = 0;
    else if (c == '\n') { xpos = 0; ypos++; }
    else if (c == '\b') { if (xpos) xpos--; }
    else if (c == '\t') {
        uint32_t spaces = 8 - (xpos & 7);
        while (spaces--) vga_putchar(' ');
    } else if (c >= 32 && c != 127) {
        video[(xpos + ypos * COLUMNS) * 2] = c;
        video[(xpos + ypos * COLUMNS) * 2 + 1] = ATTRIBUTE;
        if (++xpos >= COLUMNS) { xpos = 0; ypos++; }
    }
    if (ypos >= LINES) { vga_scroll_up(); ypos = LINES - 1; }
    cursor_set_pos(ypos, xpos);
    mm_irq_restore(flags);
}

void vga_write(tty_t *terminal)
{
    int8_t c;
    if (!terminal) return;
    while (!dequeue_head(&terminal->write, &c)) vga_putchar((uint8_t)c);
}
