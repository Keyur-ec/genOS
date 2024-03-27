#include "process.h"
#include "memory/memory.h"
#include "status.h"
#include "memory/heap/kheap.h"
#include "fs/file.h"
#include "string/string.h"
#include "kernel.h"
#include "memory/paging/paging.h"
#include "loader/formats/elf_loader.h"

/* the current process that is running */
struct process *current_process = 0;

static struct process *processes[ OS_MAX_PROCESSES ] = {};

static void process_init( struct process *process )
{
    bzero( process, sizeof( struct process ) );
}

struct process *process_current()
{
    return current_process;
}

struct process *process_get( int process_id )
{
    if( ( process_id < 0 ) || ( process_id >= OS_MAX_PROCESSES ) )
    {
        return 0;
    }

    return processes[ process_id ];
}

int process_switch( struct process *process )
{
    current_process = process;

    return OS_OK;
}

static int process_load_binary( const char *filename,
                                struct process *process )
{
    int res = OS_OK;
    void *program_data_ptr = 0;
    int fd = fopen( filename, "r" );

    if( !fd )
    {
        res = -IO_ERROR;

        if( res < 0 )
        {
            if( program_data_ptr )
            {
                kfree( program_data_ptr );
            }
        }

        fclose( fd );
        return res;
    }

    struct file_stat stat;

    res = fstat( fd, &stat );

    if( res != OS_OK )
    {
        if( res < 0 )
        {
            if( program_data_ptr )
            {
                kfree( program_data_ptr );
            }
        }

        fclose( fd );
        return res;
    }

    program_data_ptr = kzalloc( stat.filesize );

    if( !program_data_ptr )
    {
        res = -NO_MEMORY_ERROR;

        if( res < 0 )
        {
            if( program_data_ptr )
            {
                kfree( program_data_ptr );
            }
        }

        fclose( fd );
        return res;
    }

    if( fread( program_data_ptr, stat.filesize, 1, fd ) != 1 )
    {
        res = -IO_ERROR;

        if( res < 0 )
        {
            if( program_data_ptr )
            {
                kfree( program_data_ptr );
            }
        }

        fclose( fd );
        return res;
    }

    process->filetype = PROCESS_FILETYPE_BINARY;
    process->ptr      = program_data_ptr;
    process->size     = stat.filesize;

    if( res < 0 )
    {
        if( program_data_ptr )
        {
            kfree( program_data_ptr );
        }
    }

    fclose( fd );
    return res;
}

static int process_load_elf( const char *filename,
                             struct process *process )
{
    int res = OS_OK;
    struct elf_file *elf_file = 0;

    res = elf_load( filename, &elf_file );

    if( ISERR( res ) )
    {
        return res;
    }

    process->filetype = PROCESS_FILETYPE_ELF;
    process->elf_file = elf_file;

    return res;
}

static int process_load_data( const char *filename,
                              struct process *process )
{
    int res = OS_OK;

    res = process_load_elf( filename, process );

    if( res == -INVALID_FORMAT_ERROR )
    {
        res = process_load_binary( filename, process );
    }

    return res;
}

int process_map_binary( struct process *process )
{
    int res = OS_OK;

    paging_map_to( process->task->page_directory, ( void * ) OS_PROGRAM_VIRTUAL_ADDRESS, process->ptr, paging_align_address( process->ptr + process->size ), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE );

    return res;
}

int process_map_elf( struct process *process )
{
    int res = OS_OK;
    struct elf_file *elf_file = process->elf_file;
    struct elf_header *header = elf_header( elf_file );
    struct elf32_phdr *phdrs  = elf_pheader( header );

    for( int idx = 0; idx < header->e_phnum; idx++ )
    {
        struct elf32_phdr *phdr     = &phdrs[ idx ];
        void *phdr_physical_address = elf_phdr_physical_address( elf_file, phdr );
        int flags = PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL;

        if( phdr->p_flags & PF_W )
        {
            flags |= PAGING_IS_WRITEABLE;
        }

        res = paging_map_to( process->task->page_directory, paging_align_to_lower_page( ( void * ) phdr->p_vaddr ), paging_align_to_lower_page( phdr_physical_address ), paging_align_address( phdr_physical_address + phdr->p_memsz ), flags );

        if( ISERR( res ) )
        {
            break;
        }
    }

    return res;
}

int process_map_memory( struct process *process )
{
    int res = OS_OK;

    switch( process->filetype )
    {
        case PROCESS_FILETYPE_ELF:
            res = process_map_elf( process );
            break;

        case PROCESS_FILETYPE_BINARY:
            res = process_map_binary( process );
            break;

        default:
            panic( "process_map_memory: Invalid filetype\n" );
            break;
    }

    if( res < 0 )
    {
        return res;
    }

    /* finally map the stack */
    paging_map_to( process->task->page_directory, ( void * ) OS_PROGRAM_VIRTUAL_STACK_ADDRESS_END, process->stack, paging_align_address( process->stack + OS_USER_PROGRAM_STACK_SIZE ), PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL | PAGING_IS_WRITEABLE );

    return res;
}

int process_load_for_slot( const char *filename,
                           struct process **process,
                           int process_slot )
{
    int res           = OS_OK;
    struct task *task = 0;
    struct process *_process;
    void *program_stack_ptr = 0;

    if( process_get( process_slot ) != 0 )
    {
        res = -IS_TACKEN_ERROR;

        if( ISERR( res ) )
        {
            if( _process && _process->task )
            {
                task_free( _process->task );
            }

            /* free the process data */
        }

        return res;
    }

    _process = kzalloc( sizeof( struct process ) );

    if( !_process )
    {
        res = -NO_MEMORY_ERROR;

        if( ISERR( res ) )
        {
            if( _process && _process->task )
            {
                task_free( _process->task );
            }

            /* free the process data */
        }

        return res;
    }

    process_init( _process );
    res = process_load_data( filename, _process );

    if( res < 0 )
    {
        if( ISERR( res ) )
        {
            if( _process && _process->task )
            {
                task_free( _process->task );
            }

            /* free the process data */
        }

        return res;
    }

    program_stack_ptr = kzalloc( OS_USER_PROGRAM_STACK_SIZE );

    if( !program_stack_ptr )
    {
        res = -NO_MEMORY_ERROR;

        if( ISERR( res ) )
        {
            if( _process && _process->task )
            {
                task_free( _process->task );
            }

            /* free the process data */
        }

        return res;
    }

    strncpy( _process->filename, filename, sizeof( _process->filename ) );
    _process->stack = program_stack_ptr;
    _process->id    = process_slot;

    /* create a task */
    task = task_new( _process );

    if( ERROR_I( task ) == 0 )
    {
        res = ERROR_I( res );

        if( ISERR( res ) )
        {
            if( _process && _process->task )
            {
                task_free( _process->task );
            }

            /* free the process data */
        }

        return res;
    }

    _process->task = task;

    res = process_map_memory( _process );

    if( res < 0 )
    {
        if( ISERR( res ) )
        {
            if( _process && _process->task )
            {
                task_free( _process->task );
            }

            /* free the process data */
        }

        return res;
    }

    *process = _process;

    /* add the process to the array */
    processes[ process_slot ] = _process;

    if( ISERR( res ) )
    {
        if( _process && _process->task )
        {
            task_free( _process->task );
        }

        /* free the process data */
    }

    return res;
}

int process_get_free_slot()
{
    for( int idx = 0; idx < OS_MAX_PROCESSES; idx++ )
    {
        if( processes[ idx ] == 0 )
        {
            return idx;
        }
    }

    return -IS_TACKEN_ERROR;
}

int process_load( const char *filename,
                  struct process **process )
{
    int res          = OS_OK;
    int process_slot = process_get_free_slot();

    if( process_slot < 0 )
    {
        res = -IS_TACKEN_ERROR;
        return res;
    }

    res = process_load_for_slot( filename, process, process_slot );

    return res;
}

int process_load_switch( const char *filename,
                         struct process **process )
{
    int res = process_load( filename, process );

    if( res == OS_OK )
    {
        process_switch( *process );
    }

    return res;
}

static int process_find_free_allocation_index( struct process *process )
{
    int res = -NO_MEMORY_ERROR;

    for( int idx = 0; idx < OS_MAX_PROGRAMS_ALLOCATIONS; idx++ )
    {
        if( process->allocations[ idx ].ptr == 0 )
        {
            res = idx;
            break;
        }
    }

    return res;
}

void *process_malloc( struct process *process,
                      size_t size )
{
    void *ptr = kzalloc( size );

    if( !ptr )
    {
        if( ptr )
        {
            kfree( ptr );
        }

        return 0;
    }

    int index = process_find_free_allocation_index( process );

    if( index < 0 )
    {
        if( ptr )
        {
            kfree( ptr );
        }

        return 0;
    }

    int res = paging_map_to( process->task->page_directory, ptr, ptr, paging_align_address( ptr + size ), PAGING_IS_WRITEABLE | PAGING_IS_PRESENT | PAGING_ACCESS_FROM_ALL );

    if( res < 0 )
    {
        if( ptr )
        {
            kfree( ptr );
        }

        return 0;
    }

    process->allocations[ index ].ptr  = ptr;
    process->allocations[ index ].size = size;

    return ptr;
}

static bool process_is_process_pointer( struct process *process,
                                        void *ptr )
{
    for( int idx = 0; idx < OS_MAX_PROGRAMS_ALLOCATIONS; idx++ )
    {
        if( process->allocations[ idx ].ptr == ptr )
        {
            return true;
        }
    }

    return false;
}

static void process_allocation_unjoin( struct process *process,
                                       void *ptr )
{
    for( int idx = 0; idx < OS_MAX_PROGRAMS_ALLOCATIONS; idx++ )
    {
        if( process->allocations[ idx ].ptr == ptr )
        {
            process->allocations[ idx ].ptr  = 0x00;
            process->allocations[ idx ].size = 0x00;
        }
    }
}

static struct process_allocation *process_get_allocation_by_addr( struct process *process,
                                                                  void *addr )
{
    for( int idx = 0; idx < OS_MAX_PROGRAMS_ALLOCATIONS; idx++ )
    {
        if( process->allocations[ idx ].ptr == addr )
        {
            return &process->allocations[ idx ];
        }
    }

    return 0;
}

void process_free( struct process *process,
                   void *ptr )
{
    /* unlink the pages from the process for the given address */
    struct process_allocation *allocation = process_get_allocation_by_addr( process, ptr );

    if( !allocation )
    {
        /* its not our pointer */
        return;
    }

    int res = paging_map_to( process->task->page_directory, allocation->ptr, allocation->ptr, paging_align_address( allocation->ptr + allocation->size ), 0x00 );

    if( res < 0 )
    {
        return;
    }

    /* unjoin the allocation */
    process_allocation_unjoin( process, ptr );

    /* we can now free the memory */
    kfree( ptr );
}

void process_get_arguments( struct process *process,
                            int *argc,
                            char ***argv )
{
    *argc = process->arguments.argc;
    *argv = process->arguments.argv;
}

int process_count_command_arguments( struct command_argument *root_argument )
{
    struct command_argument *current = root_argument;
    int count = 0;

    while( current )
    {
        count++;
        current = current->next;
    }

    return count;
}

int process_inject_arguments( struct process *process,
                              struct command_argument *root_argument )
{
    int res = OS_OK;

    struct command_argument *current = root_argument;
    int i    = 0;
    int argc = process_count_command_arguments( root_argument );

    if( argc == 0 )
    {
        res = -IO_ERROR;
        return res;
    }

    char **argv = process_malloc( process, sizeof( const char * ) * argc );

    if( !argv )
    {
        res = -NO_MEMORY_ERROR;
        return res;
    }

    while( current )
    {
        char *argument_str = process_malloc( process, sizeof( current->argument ) );

        if( !argument_str )
        {
            res = -NO_MEMORY_ERROR;
            return res;
        }

        strncpy( argument_str, current->argument, sizeof( current->argument ) );
        argv[ i ] = argument_str;
        current   = current->next;
        i++;
    }

    process->arguments.argc = argc;
    process->arguments.argv = argv;

    return res;
}

static int process_terminate_allocations( struct process *process )
{
    for( int idx = 0; idx < OS_MAX_PROGRAMS_ALLOCATIONS; idx++ )
    {
        process_free( process, process->allocations[ idx ].ptr );
    }

    return 0;
}

static int process_free_binary_data( struct process *process )
{
    kfree( process->ptr );
    return 0;
}

static int process_free_elf_data( struct process *process )
{
    elf_close( process->elf_file );
    return 0;
}

static int process_free_program_data( struct process *process )
{
    int res = OS_OK;

    switch( process->filetype )
    {
        case PROCESS_FILETYPE_BINARY:
            res = process_free_binary_data( process );
            break;

        case PROCESS_FILETYPE_ELF:
            res = process_free_elf_data( process );
            break;

        default:
            res = -INVALID_ARGUMENT_ERROR;
            break;
    }

    return res;
}

static void process_switch_to_any()
{
    for( int idx = 0; idx < OS_MAX_PROCESSES; idx++ )
    {
        if( processes[ idx ] )
        {
            process_switch( processes[ idx ] );
            return;
        }
    }

    panic( "no process to switch!\n" );
    return;
}

static void process_unlink( struct process *process )
{
    processes[ process->id ] = 0x00;

    if( current_process == process )
    {
        process_switch_to_any();
    }
}

int process_terminate( struct process *process )
{
    int res = OS_OK;

    res = process_terminate_allocations( process );

    if( res < 0 )
    {
        return res;
    }

    res = process_free_program_data( process );

    if( res < 0 )
    {
        return res;
    }

    /* free the process stack memory */
    kfree( process->stack );
    /* free the task */
    task_free( process->task );
    /* unlink the process from the process array */
    process_unlink( process );

    return res;
}
