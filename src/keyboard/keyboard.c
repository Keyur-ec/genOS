#include "keyboard.h"
#include "status.h"
#include "kernel.h"
#include "task/process.h"
#include "task/task.h"
#include "classicPS2.h"

static struct keyboard *keyboard_list_head = 0;
static struct keyboard *keyboard_list_last = 0;

int keyboard_insert( struct keyboard *keyboard )
{
    int res = OS_OK;

    if( keyboard->init == 0 )
    {
        res = -INVALID_ARGUMENT_ERROR;
        return res;
    }

    if( keyboard_list_last )
    {
        keyboard_list_last->next = keyboard;
        keyboard_list_last       = keyboard;
        res = keyboard->init();

        return res;
    }

    keyboard_list_head = keyboard;
    keyboard_list_last = keyboard;
    res = keyboard->init();

    return res;
}

void keyboard_init()
{
    keyboard_insert( classic_init() );
}

static int keyboard_get_tail_index( struct process *process )
{
    return process->keyboard.tail % sizeof( process->keyboard.buffer );
}

void keyboard_backspace( struct process *process )
{
    process->keyboard.tail--;
    int real_index = keyboard_get_tail_index( process );

    process->keyboard.buffer[ real_index ] = 0x00;
}

void keyboard_push( char chr )
{
    struct process *process = process_current();

    if( !process )
    {
        return;
    }

    if( chr == 0x00 )
    {
        return;
    }

    int real_index = keyboard_get_tail_index( process );

    process->keyboard.buffer[ real_index ] = chr;
    process->keyboard.tail++;
}

char keyboard_pop()
{
    if( !task_current() )
    {
        return 0;
    }

    struct process *process = task_current()->process;
    int real_index          = process->keyboard.head % sizeof( process->keyboard.buffer );
    char chr = process->keyboard.buffer[ real_index ];

    if( chr == 0x00 )
    {
        /* nothing to pop */
        return 0;
    }

    process->keyboard.buffer[ real_index ] = 0x00;
    process->keyboard.head++;

    return chr;
}

void keyboard_set_capslock( struct keyboard *keyboard,
                            KAYBOARD_CAPS_LOCK_STATE state )
{
    keyboard->capslock_state = state;
}

KAYBOARD_CAPS_LOCK_STATE keyboard_get_caps_lock( struct keyboard *keyboard )
{
    return keyboard->capslock_state;
}
