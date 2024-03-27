#ifndef CLASSIC_PS2_KEYBOARD_H_
#define CLASSIC_PS2_KEYBOARD_H_

#define PS2_PORT                         0x64
#define PS2_COMMAND_ENABLE_FIRST_PORT    0xAE
#define KEYBOARD_INPUT_PORT              0x60

#define CLASSIC_KEYBOARD_KEY_RELEASED    0x80
#define ISR_KEYBOARD_INTERRUPT           0x21
#define CLASSIC_KEYBOARD_CAPSLOCK        0x3A

int classic_keyboard_init();
struct keyboard *classic_init();
void classic_keyboard_handle_interrupt();

#endif /* CLASSIC_PS2_KEYBOARD_H_ */
