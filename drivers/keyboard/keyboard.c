#include <stdint.h>
#include <keyboard.h>
#include <io.h>
#include <apic.h>
#include <printf.h>

/* PS/2 Keyboard ports */
#define KEYBOARD_DATA_PORT    0x60
#define KEYBOARD_STATUS_PORT  0x64
#define KEYBOARD_COMMAND_PORT 0x64

/* Keyboard status register bits */
#define KEYBOARD_STATUS_OUTPUT_FULL  0x01
#define KEYBOARD_STATUS_INPUT_FULL   0x02

/* Scan code types */
#define SCANCODE_MAKE  0
#define SCANCODE_BREAK 1

/* Special key codes */
#define KEY_ESC        0x01
#define KEY_BACKSPACE  0x0E
#define KEY_TAB        0x0F
#define KEY_ENTER      0x1C
#define KEY_LEFT_CTRL  0x1D
#define KEY_LEFT_SHIFT 0x2A
#define KEY_RIGHT_SHIFT 0x36
#define KEY_LEFT_ALT   0x38
#define KEY_CAPS_LOCK  0x3A
#define KEY_F1         0x3B
#define KEY_F2         0x3C
#define KEY_F3         0x3D
#define KEY_F4         0x3E
#define KEY_F5         0x3F
#define KEY_F6         0x40
#define KEY_F7         0x41
#define KEY_F8         0x42
#define KEY_F9         0x43
#define KEY_F10        0x44
#define KEY_F11        0x57
#define KEY_F12        0x58
#define KEY_NUM_LOCK   0x45
#define KEY_SCROLL_LOCK 0x46

/* Extended scan codes */
#define SCANCODE_EXTENDED 0xE0
#define SCANCODE_RELEASE  0xF0

/* Keyboard state */
static uint8_t keyboard_state = 0;
#define STATE_SHIFT    0x01
#define STATE_CTRL     0x02
#define STATE_ALT      0x04
#define STATE_CAPS     0x08
#define STATE_NUM      0x10
#define STATE_SCROLL   0x20

/* Scan code to ASCII conversion table (normal keys) */
static const char scan_code_normal[128] = {
    0,   0,   '1', '2', '3', '4', '5', '6',  /* 0x00-0x07 */
    '7', '8', '9', '0', '-', '=', 0,   0,    /* 0x08-0x0F */
    'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',  /* 0x10-0x17 */
    'o', 'p', '[', ']', 0,   0,   'a', 's',  /* 0x18-0x1F */
    'd', 'f', 'g', 'h', 'j', 'k', 'l', ';',  /* 0x20-0x27 */
    '\'', '`', 0,   '\\', 'z', 'x', 'c', 'v', /* 0x28-0x2F */
    'b', 'n', 'm', ',', '.', '/', 0,   '*',  /* 0x30-0x37 */
    0,   ' ', 0,   0,   0,   0,   0,   0,    /* 0x38-0x3F */
    0,   0,   0,   0,   0,   0,   0,   '7',  /* 0x40-0x47 */
    '8', '9', '-', '4', '5', '6', '+', '1',  /* 0x48-0x4F */
    '2', '3', '0', '.', 0,   0,   0,   0,    /* 0x50-0x57 */
    0,   0,   0,   0,   0,   0,   0,   0,    /* 0x58-0x5F */
    0,   0,   0,   0,   0,   0,   0,   0,    /* 0x60-0x67 */
    0,   0,   0,   0,   0,   0,   0,   0,    /* 0x68-0x6F */
    0,   0,   0,   0,   0,   0,   0,   0,    /* 0x70-0x77 */
    0,   0,   0,   0,   0,   0,   0,   0,    /* 0x78-0x7F */
};

/* Scan code to ASCII conversion table (shifted keys) */
static const char scan_code_shift[128] = {
    0,   0,   '!', '@', '#', '$', '%', '^',  /* 0x00-0x07 */
    '&', '*', '(', ')', '_', '+', 0,   0,    /* 0x08-0x0F */
    'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',  /* 0x10-0x17 */
    'O', 'P', '{', '}', 0,   0,   'A', 'S',  /* 0x18-0x1F */
    'D', 'F', 'G', 'H', 'J', 'K', 'L', ':',  /* 0x20-0x27 */
    '"', '~', 0,   '|', 'Z', 'X', 'C', 'V',  /* 0x28-0x2F */
    'B', 'N', 'M', '<', '>', '?', 0,   '*',  /* 0x30-0x37 */
    0,   ' ', 0,   0,   0,   0,   0,   0,    /* 0x38-0x3F */
    0,   0,   0,   0,   0,   0,   0,   '7',  /* 0x40-0x47 */
    '8', '9', '-', '4', '5', '6', '+', '1',  /* 0x48-0x4F */
    '2', '3', '0', '.', 0,   0,   0,   0,    /* 0x50-0x57 */
    0,   0,   0,   0,   0,   0,   0,   0,    /* 0x58-0x5F */
    0,   0,   0,   0,   0,   0,   0,   0,    /* 0x60-0x67 */
    0,   0,   0,   0,   0,   0,   0,   0,    /* 0x68-0x6F */
    0,   0,   0,   0,   0,   0,   0,   0,    /* 0x70-0x77 */
    0,   0,   0,   0,   0,   0,   0,   0,    /* 0x78-0x7F */
};

/* Buffer for keyboard input */
#define KEYBOARD_BUFFER_SIZE 256
static char keyboard_buffer[KEYBOARD_BUFFER_SIZE];
static uint32_t keyboard_buffer_head = 0;
static uint32_t keyboard_buffer_tail = 0;

/* Internal state */
static uint8_t extended_scan = 0;
static uint8_t release_scan = 0;

/* Wait for keyboard controller to be ready */
static void keyboard_wait(void) {
    uint32_t timeout = 100000;
    while (timeout--) {
        if (!(inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_INPUT_FULL)) {
            return;
        }
    }
}

/* Send command to keyboard controller */
static void keyboard_send_command(uint8_t cmd) {
    keyboard_wait();
    outb(KEYBOARD_COMMAND_PORT, cmd);
}


/* Send data to keyboard */
static void keyboard_send_data(uint8_t data) {
    keyboard_wait();
    outb(KEYBOARD_DATA_PORT, data);
}

/* Initialize keyboard */
void keyboard_init(void) {
    /* Enable keyboard interrupt */
    keyboard_send_command(0xAE);  /* Enable first PS/2 port */
    
    /* Flush keyboard buffer */
    while (inb(KEYBOARD_STATUS_PORT) & KEYBOARD_STATUS_OUTPUT_FULL) {
        inb(KEYBOARD_DATA_PORT);
    }

    ioapic_unmask_interrupt(IRQ_KEYBOARD);
    
    /* Reset keyboard state */
    keyboard_state = 0;
    keyboard_buffer_head = 0;
    keyboard_buffer_tail = 0;
    extended_scan = 0;
    release_scan = 0;
}

/* Process scan code */
static void keyboard_process_scan_code(uint8_t scan_code) {
    char ch = 0;
    
    /* Handle extended scan codes */
    if (scan_code == SCANCODE_EXTENDED) {
        extended_scan = 1;
        return;
    }
    
    /* Handle release scan codes */
    if (scan_code == SCANCODE_RELEASE) {
        release_scan = 1;
        return;
    }
    
    /* Check if this is a break code (key release) */
    if (release_scan || (scan_code & 0x80)) {
        release_scan = 0;
        scan_code &= 0x7F;
        
        /* Handle modifier key releases */
        switch (scan_code) {
        case KEY_LEFT_SHIFT:
        case KEY_RIGHT_SHIFT:
            keyboard_state &= ~STATE_SHIFT;
            break;
        case KEY_LEFT_CTRL:
            keyboard_state &= ~STATE_CTRL;
            break;
        case KEY_LEFT_ALT:
            keyboard_state &= ~STATE_ALT;
            break;
        }
        extended_scan = 0;
        return;
    }
    
    /* Handle modifier key presses */
    switch (scan_code) {
    case KEY_LEFT_SHIFT:
    case KEY_RIGHT_SHIFT:
        keyboard_state |= STATE_SHIFT;
        extended_scan = 0;
        return;
    case KEY_LEFT_CTRL:
        keyboard_state |= STATE_CTRL;
        extended_scan = 0;
        return;
    case KEY_LEFT_ALT:
        keyboard_state |= STATE_ALT;
        extended_scan = 0;
        return;
    case KEY_CAPS_LOCK:
        keyboard_state ^= STATE_CAPS;
        extended_scan = 0;
        return;
    case KEY_NUM_LOCK:
        keyboard_state ^= STATE_NUM;
        extended_scan = 0;
        return;
    case KEY_SCROLL_LOCK:
        keyboard_state ^= STATE_SCROLL;
        extended_scan = 0;
        return;
    }
    
    /* Convert scan code to character */
    if (scan_code < 128) {
        uint8_t shift = (keyboard_state & STATE_SHIFT) ? 1 : 0;
        uint8_t caps = (keyboard_state & STATE_CAPS) ? 1 : 0;
        
        if (shift || caps) {
            ch = scan_code_shift[scan_code];
        } else {
            ch = scan_code_normal[scan_code];
        }
        
        /* Handle special keys */
        if (ch == 0) {
            switch (scan_code) {
            case KEY_ENTER:
                ch = '\n';
                break;
            case KEY_TAB:
                ch = '\t';
                break;
            case KEY_BACKSPACE:
                ch = '\b';
                break;
            case KEY_ESC:
                ch = 0x1B;  /* ESC character */
                break;
            }
        }
        
        /* Add character to buffer if valid */
        if (ch != 0) {
            uint32_t next_tail = (keyboard_buffer_tail + 1) % KEYBOARD_BUFFER_SIZE;
            if (next_tail != keyboard_buffer_head) {
                keyboard_buffer[keyboard_buffer_tail] = ch;
                keyboard_buffer_tail = next_tail;
            }
        }
        printf("%c   ",ch);
    }
    
    extended_scan = 0;
}

/* Keyboard interrupt handler */
void keyboard_interrupt_handler(void) {
    uint8_t scan_code;
    
    /* Read scan code from keyboard data port */
    scan_code = inb(KEYBOARD_DATA_PORT);
    
    /* Process scan code */
    keyboard_process_scan_code(scan_code);
    
    /* Send EOI to APIC */
    lapic_eoi();
}

/* Check if keyboard buffer has data */
int keyboard_has_data(void) {
    return keyboard_buffer_head != keyboard_buffer_tail;
}

/* Read character from keyboard buffer */
char keyboard_read_char(void) {
    if (!keyboard_has_data()) {
        return 0;
    }
    
    char ch = keyboard_buffer[keyboard_buffer_head];
    keyboard_buffer_head = (keyboard_buffer_head + 1) % KEYBOARD_BUFFER_SIZE;
    return ch;
}

/* Get keyboard state */
uint8_t keyboard_get_state(void) {
    return keyboard_state;
}

