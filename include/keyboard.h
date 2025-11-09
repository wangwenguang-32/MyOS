#ifndef KEYBOARD_H
#define KEYBOARD_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Initialize keyboard */
void keyboard_init(void);

/* Keyboard interrupt handler */
void keyboard_interrupt_handler(void);

/* Check if keyboard buffer has data */
int keyboard_has_data(void);

/* Read character from keyboard buffer */
char keyboard_read_char(void);

/* Get keyboard state (modifier keys) */
uint8_t keyboard_get_state(void);

/* Keyboard state flags */
#define KEYBOARD_STATE_SHIFT    0x01
#define KEYBOARD_STATE_CTRL     0x02
#define KEYBOARD_STATE_ALT      0x04
#define KEYBOARD_STATE_CAPS     0x08
#define KEYBOARD_STATE_NUM      0x10
#define KEYBOARD_STATE_SCROLL   0x20

#ifdef __cplusplus
}
#endif

#endif /* KEYBOARD_H */

