#ifndef OS_H_
#define OS_H_

#include <stddef.h>
#include <stdbool.h>

#define INVALID_COMMAND_ARGUMENT    10

struct command_argument
{
    char argument[ 512 ];
    struct command_argument *next;
};

struct process_arguments
{
    int argc;
    char **argv;
};

void print( const char *filename );
int os_getkey();
int os_putchar( int chr );
void *os_malloc( size_t size );
void os_free( void *ptr );
int os_system( struct command_argument *arguments );
void os_process_get_arguments( struct process_arguments *arguments );
void os_exit();

int os_getkey_block();
void os_terminal_readline( char *out,
                           int max,
                           bool output_while_typing );
struct command_argument *os_parse_command( const char *command,
                                           int max );
int os_system_run( const char *command );
void os_process_load_start( const char *filename );

#endif /* OS_H_ */
