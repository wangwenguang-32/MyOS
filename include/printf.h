#ifndef PRINTF_H
#define PRINTF_H
/*  Some screen stuff. */
/*  The number of columns. */
#define COLUMNS                 80
/*  The number of lines. */
#define LINES                   24
/*  The attribute of an character. */
#define ATTRIBUTE               7
/*  The video memory address. */
#define VIDEO                   0xC00B8000


void cls (void);

void printf (const char *format, ...);

#endif
