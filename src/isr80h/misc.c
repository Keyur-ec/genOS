#include "misc.h"
#include "idt/idt.h"
#include "task/task.h"

void *isr80h_command0_sum( struct interrupt_frame *frame )
{
    int var2 = ( int ) task_get_stack_item( task_current(), 1 );
    int var1 = ( int ) task_get_stack_item( task_current(), 0 );

    return ( void * ) ( var1 + var2 );
}
