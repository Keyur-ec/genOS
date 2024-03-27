#include "idt.h"
#include "config.h"
#include "kernel.h"
#include "memory/memory.h"
#include "io/io.h"
#include "task/task.h"
#include "status.h"
#include "task/process.h"

struct idt_desc idt_descriptors[ OS_TOTAL_INTERRUPTS ];
struct idtr_desc idtr_descriptor;

extern void idt_load( struct idtr_desc *ptr );
extern void int21h();
extern void no_interrupt();
extern void isr80h_wrapper();

extern void *interrupt_pointer_table[ OS_TOTAL_INTERRUPTS ];

static INTERRUPT_CALLBACK_FUNCTION interrupt_callbacks[ OS_TOTAL_INTERRUPTS ];
static ISR80H_COMMAND isr80h_commands[ OS_MAX_ISR80H_COMMANDS ];

void no_interrupt_handler()
{
    /* interrupt acknowledgement */
    outb( 0x20, 0x20 );
}

void interrupt_handler( int interrupt,
                        struct interrupt_frame *frame )
{
    kernel_page();

    if( interrupt_callbacks[ interrupt ] != 0 )
    {
        save_task_current_state( frame );
        interrupt_callbacks[ interrupt ]( frame );
    }

    task_page();

    /* interrupt acknowledgement */
    outb( 0x20, 0x20 );
}

void idt_zero()
{
    print( "divide by zero error\n" );
}

void idt_set( int interrupt_no,
              void *address )
{
    struct idt_desc *desc = &idt_descriptors[ interrupt_no ];

    desc->offset_1  = ( uint32_t ) address & LOWER_OFFSET_ADDRESS_MASK;
    desc->selector  = KERNEL_CODE_SELECTOR;
    desc->zero      = UNUSED_FIELD;
    desc->type_attr = 0xEE;

    /* desc->type_attr = IDT_TYPE_INTERRUPT_GATE_32BIT; */
    /* desc->storage_seg = INTERRUPT_AND_TRAP; */
    /* desc->dpl = PRIVILEGE_LEVEL; */
    /* desc->present = ~UNUSED; */

    desc->offset_2 = ( uint32_t ) address >> 16;
}

void idt_handle_exception()
{
    process_terminate( task_current()->process );
    task_next();
}

void idt_init()
{
    memset( idt_descriptors, 0, sizeof( idt_descriptors ) );

    idtr_descriptor.limit = sizeof( idt_descriptors ) - 1;
    idtr_descriptor.base  = ( uint32_t ) idt_descriptors;

    for( int i = 0; i < OS_TOTAL_INTERRUPTS; i++ )
    {
        idt_set( i, interrupt_pointer_table[ i ] );
    }

    idt_set( 0, idt_zero );
    idt_set( 0x80, isr80h_wrapper );

    for( int i = 0; i < 0x20; i++ )
    {
        idt_register_interrupt_callback( i, idt_handle_exception );
    }

    idt_register_interrupt_callback( 0x20, idt_clock );

    /* load the interrupt descriptor table */
    idt_load( &idtr_descriptor );
}

void isr80h_register_command( int command_id,
                              ISR80H_COMMAND command )
{
    if( ( command_id < 0 ) || ( command_id >= OS_MAX_ISR80H_COMMANDS ) )
    {
        panic( "the command is out of bound!!\n" );
    }

    if( isr80h_commands[ command_id ] )
    {
        panic( "you are attempting to overwrite an existing command!!\n" );
    }

    isr80h_commands[ command_id ] = command;
}

void *isr80h_handle_command( int command,
                             struct interrupt_frame *frame )
{
    void *res = 0;

    if( ( command < 0 ) || ( command >= OS_MAX_ISR80H_COMMANDS ) )
    {
        /* invalid command */
        return 0;
    }

    ISR80H_COMMAND command_func = isr80h_commands[ command ];

    if( !command_func )
    {
        return 0;
    }

    res = command_func( frame );

    return res;
}

void *isr80h_handler( int command,
                      struct interrupt_frame *frame )
{
    void *res = 0;

    kernel_page();
    save_task_current_state( frame );
    res = isr80h_handle_command( command, frame );
    task_page();

    return res;
}

int idt_register_interrupt_callback( int interrupt,
                                     INTERRUPT_CALLBACK_FUNCTION interrupt_callback )
{
    if( ( interrupt < 0 ) || ( interrupt >= OS_TOTAL_INTERRUPTS ) )
    {
        return -INVALID_ARGUMENT_ERROR;
    }

    interrupt_callbacks[ interrupt ] = interrupt_callback;

    return OS_OK;
}

void idt_clock()
{
    /* interrupt acknowledgement */
    outb( 0x20, 0x20 );

    /* switch to the next task */
    task_next();
}
