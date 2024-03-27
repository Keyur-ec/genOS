#ifndef KERNEL_H_
#define KERNEL_H_

#define VGA_WIDTH         80
#define VGA_HEIGHT        20
#define VIDEO_MEM_ADDR    0xB8000

#define BLACK_COLOR       0
#define WHITE_COLOR       15

#define ERROR( value )      ( void * ) ( value )
#define ERROR_I( value )    ( int ) ( value )
#define ISERR( value )      ( ( int ) value < 0 )

#include <stdint.h>

void print( const char *str );
void panic( const char *msg );
void kernel_page();
void kernel_registers();
void kernel_main();
void terminal_writechar( char chr,
                         uint8_t color );

#endif /* KERNEL_H_ */
