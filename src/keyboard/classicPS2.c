#include "classicPS2.h"
#include "keyboard.h"
#include "status.h"
#include "io/io.h"
#include "kernel.h"
#include "idt/idt.h"
#include "task/task.h"
#include <stdint.h>
#include <stddef.h>

static uint8_t keyboard_scan_set_one[] =
{
    0x00, 0x1B, '1',  '2',  '3',  '4',  '5',
    '6',  '7',  '8',  '9',  '0',  '-',  '=',
    0x08, '\t', 'Q',  'W',  'E',  'R',  'T',
    'Y',  'U',  'I',  'O',  'P',  '[',  ']',
    0x0D, 0x00, 'A',  'S',  'D',  'F',  'G',
    'H',  'J',  'K',  'L',  ';',  '\'', '`',
    0x00, '\\', 'Z',  'X',  'C',  'V',  'B',
    'N',  'M',  ',',  '.',  '/',  0x00, '*',
    0x00, 0x20, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, '7',  '8',  '9',  '-',  '4',  '5',
    '6',  '+',  '1',  '2',  '3',  '0',  '.'
};

struct keyboard classic_keyboard =
{
    .name = "PS2 keyboard",
    .init = classic_keyboard_init
};

int classic_keyboard_init()
{
    idt_register_interrupt_callback( ISR_KEYBOARD_INTERRUPT, classic_keyboard_handle_interrupt );
    keyboard_set_capslock( &classic_keyboard, KEYBOARD_CAPS_LOCK_OFF );
    outb( PS2_PORT, PS2_COMMAND_ENABLE_FIRST_PORT );

    return OS_OK;
}

uint8_t classic_keyboard_scan_code_to_char( uint8_t scancode )
{
    size_t size_of_keyboard_set_one = sizeof( keyboard_scan_set_one ) / sizeof( uint8_t );

    if( scancode > size_of_keyboard_set_one )
    {
        return 0;
    }

    char chr = keyboard_scan_set_one[ scancode ];

    if( keyboard_get_caps_lock( &classic_keyboard ) == KEYBOARD_CAPS_LOCK_OFF )
    {
        if( ( chr >= 'A' ) && ( chr <= 'Z' ) )
        {
            chr += 32;
        }
    }

    return chr;
}

void classic_keyboard_handle_interrupt()
{
    kernel_page();

    uint8_t scancode = 0;

    scancode = insb( KEYBOARD_INPUT_PORT );
    insb( KEYBOARD_INPUT_PORT );

    if( scancode & CLASSIC_KEYBOARD_KEY_RELEASED )
    {
        return;
    }

    if( scancode == CLASSIC_KEYBOARD_CAPSLOCK )
    {
        KAYBOARD_CAPS_LOCK_STATE capslock_state = keyboard_get_caps_lock( &classic_keyboard );
        keyboard_set_capslock( &classic_keyboard, capslock_state == KEYBOARD_CAPS_LOCK_ON ? KEYBOARD_CAPS_LOCK_OFF : KEYBOARD_CAPS_LOCK_ON );
    }

    uint8_t chr = classic_keyboard_scan_code_to_char( scancode );

    if( chr != 0 )
    {
        keyboard_push( chr );
    }

    task_page();
}

struct keyboard *classic_init()
{
    return &classic_keyboard;
}