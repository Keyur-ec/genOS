#include "kernel.h"
#include <stddef.h>
#include "idt/idt.h"
#include "io/io.h"
#include "memory/heap/kheap.h"
#include "memory/paging/paging.h"
#include "string/string.h"
#include "disk/disk.h"
#include "fs/path_parser.h"
#include "disk/disk_streamer.h"
#include "fs/file.h"
#include "gdt/gdt.h"
#include "config.h"
#include "memory/memory.h"
#include "task/tss.h"
#include "task/task.h"
#include "task/process.h"
#include "status.h"
#include "isr80h/isr80h.h"
#include "keyboard/keyboard.h"

static uint16_t *video_mem   = 0;
static uint16_t terminal_row = 0;
static uint16_t terminal_col = 0;

static uint16_t terminal_make_char( char chr,
                                    uint8_t color )
{
    return ( color << 8 ) | chr;
}

static void terminal_putchar( int xCord,
                              int yCord,
                              char chr,
                              uint8_t color )
{
    video_mem[ ( yCord * VGA_WIDTH ) + xCord ] = terminal_make_char( chr, color );
}

static void terminal_backspace()
{
    if( ( terminal_row == 0 ) && ( terminal_col == 0 ) )
    {
        return;
    }

    if( terminal_col == 0 )
    {
        terminal_row -= 1;
        terminal_col  = VGA_WIDTH;
    }

    terminal_col -= 1;
    terminal_writechar( ' ', WHITE_COLOR );
    terminal_col -= 1;
}

void terminal_writechar( char chr,
                         uint8_t color )
{
    if( chr == '\n' )
    {
        terminal_col  = 0;
        terminal_row += 1;
        return;
    }

    if( chr == 0x08 )
    {
        terminal_backspace();
        return;
    }

    terminal_putchar( terminal_col, terminal_row, chr, color );
    terminal_col += 1;

    if( terminal_col >= VGA_WIDTH )
    {
        terminal_col  = 0;
        terminal_row += 1;
    }
}

static void terminal_init()
{
    video_mem    = ( uint16_t * ) VIDEO_MEM_ADDR;
    terminal_row = 0;
    terminal_col = 0;

    for( int yCord = 0; yCord < VGA_HEIGHT; yCord++ )
    {
        for( int xCord = 0; xCord < VGA_WIDTH; xCord++ )
        {
            terminal_putchar( xCord, yCord, ' ', BLACK_COLOR );
        }
    }
}

void print( const char *str )
{
    size_t len = strlen( str );

    for( int index = 0; index < len; index++ )
    {
        terminal_writechar( str[ index ], WHITE_COLOR );
    }
}

static struct paging_chunk *kernel_chunk = 0;

void panic( const char *msg )
{
    print( msg );

    while( 1 )
    {
    }
}

void kernel_page()
{
    kernel_registers();
    paging_switch( kernel_chunk );
}

struct tss tss;
struct gdt gdt_real[ OS_TOTAL_GDT_SEGMENTS ];
struct gdt_structured gdt_structured[ OS_TOTAL_GDT_SEGMENTS ] =
{
    /* NULL segment */
    { .base = 0x00,              .limit = 0x00,          .type = 0x00 },
    /* kernel code segment */
    { .base = 0x00,              .limit = 0xFFFFFFFF,    .type = 0x9A },
    /* kernel data segment */
    { .base = 0x00,              .limit = 0xFFFFFFFF,    .type = 0x92 },
    /* user code segment */
    { .base = 0x00,              .limit = 0xFFFFFFFF,    .type = 0xF8 },
    /* user data segment */
    { .base = 0x00,              .limit = 0xFFFFFFFF,    .type = 0xF2 },
    /* TSS segment */
    { .base = ( uint32_t ) &tss, .limit = sizeof( tss ), .type = 0xE9 }
};

void kernel_main()
{
    terminal_init();

    bzero( gdt_real, sizeof( gdt_real ) );
    gdt_structured_to_gdt( gdt_real, gdt_structured, OS_TOTAL_GDT_SEGMENTS );

    /* load the gdt  */
    gdt_load( gdt_real, sizeof( gdt_real ) );

    /* initialize the heap */
    kheap_init();

    /* initialize the filesystems */
    fs_init();

    /* search and init the disks */
    disk_search_and_init();

    /* initialize the interrupt descriptor table */
    idt_init();

    /* setup the TSS */
    bzero( &tss, sizeof( tss ) );
    tss.esp0 = 0x600000;
    tss.ss0  = KERNEL_DATA_SELECTOR;

    /* load the TSS */
    tss_load( 0x28 );

    /* setup paging */
    kernel_chunk = paging_new( PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL );

    /* switch to kernel paging chunk */
    paging_switch( kernel_chunk );

    /* enable paging */
    enable_paging();

    /* / * enebling interrupts * / */
    /* enable_interrupts(); */

    /* register the kernel commands */
    isr80h_register_commands();

    /* initialize all the system keyboard */
    keyboard_init();

    /* ====================================================================== */
    struct process *process = 0;
    int res = process_load_switch( "0:/blank.elf", &process );

    if( res != OS_OK )
    {
        panic( "unable to load process!!\n" );
    }

    struct command_argument argument;

    strcpy( argument.argument, "Testing!" );
    argument.next = 0x00;

    process_inject_arguments( process, &argument );


    /* ====================================================================== */
    int res1 = process_load_switch( "0:/blank.elf", &process );

    if( res1 != OS_OK )
    {
        panic( "unable to load process!!\n" );
    }

    strcpy( argument.argument, "new!" );
    argument.next = 0x00;

    process_inject_arguments( process, &argument );
    task_run_first_ever_task();

    while( 1 )
    {
    }
}
