#ifndef KEYBOARD_H_
#define KEYBOARD_H_

#define KEYBOARD_CAPS_LOCK_OFF    0
#define KEYBOARD_CAPS_LOCK_ON     1

typedef int KAYBOARD_CAPS_LOCK_STATE;

typedef int (*KEYBOARD_INIT_FUCTION)();

struct process;

struct keyboard
{
    KEYBOARD_INIT_FUCTION init;
    char name[ 20 ];
    struct keyboard *next;

    KAYBOARD_CAPS_LOCK_STATE capslock_state;
};

void keyboard_init();
int keyboard_insert( struct keyboard *keyboard );
void keyboard_backspace( struct process *process );
void keyboard_push( char chr );
char keyboard_pop();
void keyboard_set_capslock( struct keyboard *keyboard,
                            KAYBOARD_CAPS_LOCK_STATE state );
KAYBOARD_CAPS_LOCK_STATE keyboard_get_caps_lock( struct keyboard *keyboard );

#endif /* KEYBOARD_H_ */
