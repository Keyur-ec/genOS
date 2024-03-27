#include "task.h"
#include "kernel.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "memory/memory.h"
#include "process.h"
#include "idt/idt.h"
#include "memory/paging/paging.h"
#include "string/string.h"
#include "loader/formats/elf_loader.h"

/* the current task that is running */
struct task *current_task = 0;

/* task linked list */
struct task *task_tail = 0;
struct task *task_head = 0;

struct task *task_current()
{
    return current_task;
}

int task_init( struct task *task,
               struct process *process )
{
    int res = OS_OK;

    bzero( task, sizeof( struct task ) );

    /* map the entire address space to its self */
    task->page_directory = paging_new( PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL );

    if( !task->page_directory )
    {
        res = -IO_ERROR;
        return res;
    }

    task->registers.ip = OS_PROGRAM_VIRTUAL_ADDRESS;

    if( process->filetype == PROCESS_FILETYPE_ELF )
    {
        task->registers.ip = elf_header( process->elf_file )->e_entry;
    }

    task->registers.ss  = USER_DATA_SEGMENT;
    task->registers.cs  = USER_CODE_SEGMENT;
    task->registers.esp = OS_PROGRAM_VIRTUAL_STACK_ADDRESS_START;

    task->process = process;

    return res;
}

struct task *task_get_next()
{
    if( !current_task->next )
    {
        return task_head;
    }

    return current_task->next;
}

static void task_list_remove( struct task *task )
{
    if( task->prev )
    {
        task->prev->next = task->next;
    }

    if( task == task_head )
    {
        task_head = task->next;
    }

    if( task == task_tail )
    {
        task_tail = task->prev;
    }

    if( task == current_task )
    {
        current_task = task_get_next();
    }
}

int task_free( struct task *task )
{
    paging_free( task->page_directory );

    task_list_remove( task );

    /* finally free the task data */
    kfree( task );

    return OS_OK;
}

struct task *task_new( struct process *process )
{
    int res = OS_OK;

    struct task *task = kzalloc( sizeof( struct task ) );

    if( !task )
    {
        res = -NO_MEMORY_ERROR;

        if( ISERR( res ) )
        {
            task_free( task );
            return ERROR( res );
        }
    }

    res = task_init( task, process );

    if( res != OS_OK )
    {
        if( ISERR( res ) )
        {
            task_free( task );
            return ERROR( res );
        }
    }

    if( task_head == 0 )
    {
        task_head    = task;
        task_tail    = task;
        current_task = task;

        if( ISERR( res ) )
        {
            task_free( task );
            return ERROR( res );
        }
    }

    task_tail->next = task;
    task->prev      = task_tail;
    task_tail       = task;

    if( ISERR( res ) )
    {
        task_free( task );
        return ERROR( res );
    }

    return task;
}

int task_switch( struct task *task )
{
    current_task = task;
    paging_switch( task->page_directory );

    return OS_OK;
}

int task_page()
{
    user_registers();
    task_switch( current_task );

    return OS_OK;
}

void task_run_first_ever_task()
{
    if( !current_task )
    {
        panic( "void task_run_first_ever_task() : no current task exists!\n" );
    }

    task_switch( task_head );
    task_return( &task_head->registers );
}

void save_task_state( struct task *task,
                      struct interrupt_frame *frame )
{
    task->registers.ip     = frame->ip;
    task->registers.cs     = frame->cs;
    task->registers.flages = frame->flags;
    task->registers.esp    = frame->esp;
    task->registers.ss     = frame->ss;
    task->registers.eax    = frame->eax;
    task->registers.ebp    = frame->ebp;
    task->registers.ebx    = frame->ebx;
    task->registers.ecx    = frame->ecx;
    task->registers.edi    = frame->edi;
    task->registers.edx    = frame->edx;
    task->registers.esi    = frame->esi;
}

void save_task_current_state( struct interrupt_frame *frame )
{
    if( !task_current() )
    {
        panic( "no current task to save!!\n" );
    }

    struct task *task = task_current();

    save_task_state( task, frame );
}

int copy_string_from_task( struct task *task,
                           void *virtual,
                           void *physical,
                           int size )
{
    if( size >= PAGING_PAGE_SIZE )
    {
        return -NO_MEMORY_ERROR;
    }

    int res    = OS_OK;
    char *temp = kzalloc( size );

    if( !temp )
    {
        res = -NO_MEMORY_ERROR;
        return res;
    }

    uint32_t *task_directory = task->page_directory->directory_entry;
    uint32_t old_entry       = paging_get( task_directory, temp );

    paging_map( task->page_directory, temp, temp, PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL );
    paging_switch( task->page_directory );
    strncpy( temp, virtual, size );
    kernel_page();

    res = paging_set( task_directory, temp, old_entry );

    if( res < 0 )
    {
        res = -IO_ERROR;
        kfree( temp );
        return res;
    }

    strncpy( physical, temp, size );
    kfree( temp );

    return res;
}

int task_page_task( struct task *task )
{
    user_registers();
    paging_switch( task->page_directory );
    return OS_OK;
}

void *task_get_stack_item( struct task *task,
                           int index )
{
    void *res = OS_OK;

    uint32_t *sp_ptr = ( uint32_t * ) task->registers.esp;

    /* switch to given tasks page */
    task_page_task( task );

    res = ( void * ) sp_ptr[ index ];

    /* switch back to the kernel page */
    kernel_page();

    return res;
}

void *task_virtual_address_to_physical( struct task *task,
                                        void *virtual_address )
{
    return paging_get_physical_address( task->page_directory->directory_entry, virtual_address );
}

void task_next()
{
    struct task *next_task = task_get_next();

    if( !next_task )
    {
        panic( "no more task to run!\n" );
    }

    task_switch( next_task );
    task_return( &next_task->registers );
}
