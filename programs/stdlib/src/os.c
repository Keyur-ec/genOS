#include "os.h"
#include "string.h"

int os_getkey_block()
{
    int val = 0;

    do
    {
        val = os_getkey();
    }
    while( val == 0 );

    return val;
}

void os_terminal_readline( char *out,
                           int max,
                           bool output_while_typing )
{
    int idx = 0;

    for( idx = 0; idx < max - 1; idx++ )
    {
        char key = os_getkey_block();

        /* carriage return means we have read the line */
        if( key == 13 )
        {
            break;
        }

        if( output_while_typing )
        {
            os_putchar( key );
        }

        /* backspace */
        if( ( key == 0x08 ) && ( idx >= 1 ) )
        {
            out[ idx - 1 ] = 0x00;
            /* -2 because we will +1 when we go to the continue */
            idx -= 2;
            continue;
        }

        out[ idx ] = key;
    }

    /* add the null terminator */
    out[ idx ] = 0x00;
}

struct command_argument *os_parse_command( const char *command,
                                           int max )
{
    struct command_argument *root_command = 0;
    char command_size[ 1024 + 1 ];

    if( max >= ( int ) sizeof( command_size ) )
    {
        return 0;
    }

    strncpy( command_size, command, sizeof( command_size ) );
    char *tocken = strtok( command_size, " " );

    if( !tocken )
    {
        return root_command;
    }

    root_command = os_malloc( sizeof( struct command_argument ) );

    if( !root_command )
    {
        return root_command;
    }

    strncpy( root_command->argument, tocken, sizeof( root_command->argument ) );
    root_command->next = 0x00;

    struct command_argument *current = root_command;

    tocken = strtok( NULL, " " );

    while( tocken != 0 )
    {
        struct command_argument *new_command = os_malloc( sizeof( struct command_argument ) );

        if( !new_command )
        {
            break;
        }

        strncpy( new_command->argument, tocken, sizeof( new_command->argument ) );
        new_command->next = 0x00;
        current->next     = new_command;
        current           = new_command;
        tocken            = strtok( NULL, " " );
    }

    return root_command;
}

int os_system_run( const char *command )
{
    char buffer[ 1024 ];

    strncpy( buffer, command, sizeof( buffer ) );
    struct command_argument *root_command_argument = os_parse_command( buffer, sizeof( buffer ) );

    if( !root_command_argument )
    {
        return -INVALID_COMMAND_ARGUMENT;
    }

    return os_system( root_command_argument );
}
