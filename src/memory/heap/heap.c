#include "heap.h"
#include "kernel.h"
#include <stdbool.h>
#include "status.h"
#include "memory/memory.h"

static int heap_validate_table( void *ptr,
                                void *end,
                                struct heap_table *table )
{
    int res = OS_OK;

    size_t table_size   = ( size_t ) ( end - ptr );
    size_t total_blocks = table_size / OS_HEAP_BLOCK_SIZE;

    if( table->total_entries != total_blocks )
    {
        res = -INVALID_ARGUMENT_ERROR;
        return res;
    }

    return res;
}

static bool heap_validate_alignment( void *ptr )
{
    return ( ( unsigned int ) ptr % OS_HEAP_BLOCK_SIZE ) == 0;
}

int heap_create( struct heap *heap,
                 void *ptr,
                 void *end,
                 struct heap_table *table )
{
    int res = OS_OK;

    if( !heap_validate_alignment( ptr ) || !heap_validate_alignment( end ) )
    {
        res = -INVALID_ARGUMENT_ERROR;
        return res;
    }

    memset( heap, 0, sizeof( struct heap ) );
    heap->start_addr = ptr;
    heap->table      = table;

    res = heap_validate_table( ptr, end, table );

    if( res < 0 )
    {
        return res;
    }

    size_t table_size = sizeof( HEAP_BLOCK_TABLE_ENTRY ) * table->total_entries;

    memset( table->entries, HEAP_BLOCK_TABLE_ENTRY_FREE, table_size );

    return res;
}

static size_t heap_align_value_to_upper( uint32_t size )
{
    if( ( size % OS_HEAP_BLOCK_SIZE ) == 0 )
    {
        return size;
    }

    size  = ( size - ( size % OS_HEAP_BLOCK_SIZE ) );
    size += OS_HEAP_BLOCK_SIZE;

    return size;
}

static int heap_get_entry_type( HEAP_BLOCK_TABLE_ENTRY entry )
{
    return entry & 0x0F;
}

int heap_get_start_block( struct heap *heap,
                          uint32_t total_blocks )
{
    struct heap_table *table = heap->table;
    int current_block        = 0;
    int block_start          = -1;

    for( size_t looking_free = 0; looking_free < table->total_entries; looking_free++ )
    {
        if( heap_get_entry_type( table->entries[ looking_free ] != HEAP_BLOCK_TABLE_ENTRY_FREE ) )
        {
            current_block = 0;
            block_start   = -1;
            continue;
        }

        /* if this is the first block */
        if( block_start == -1 )
        {
            block_start = looking_free;
        }

        current_block++;

        if( current_block == total_blocks )
        {
            break;
        }
    }

    if( block_start == -1 )
    {
        return -NO_MEMORY_ERROR;
    }

    return block_start;
}

void *heap_block_to_address( struct heap *heap,
                             int block )
{
    return heap->start_addr + ( block * OS_HEAP_BLOCK_SIZE );
}

void heap_mark_blocks_tacken( struct heap *heap,
                              int start_block,
                              int total_block )
{
    int end_block = ( start_block + total_block ) - 1;

    HEAP_BLOCK_TABLE_ENTRY entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN | HEAP_BLOCK_IS_FRIST;

    if( total_block > 1 )
    {
        entry |= HEAP_BLOCK_HAS_NEXT;
    }

    for( int entry_idx = start_block; entry_idx <= end_block; entry_idx++ )
    {
        heap->table->entries[ entry_idx ] = entry;
        entry = HEAP_BLOCK_TABLE_ENTRY_TAKEN;

        if( entry_idx != end_block - 1 )
        {
            entry |= HEAP_BLOCK_HAS_NEXT;
        }
    }
}

void *heap_malloc_blocks( struct heap *heap,
                          uint32_t total_blocks )
{
    void *address = 0;

    int start_block = heap_get_start_block( heap, total_blocks );

    if( start_block < 0 )
    {
        return address;
    }

    address = heap_block_to_address( heap, start_block );

    /* mark the block as taken */
    heap_mark_blocks_tacken( heap, start_block, total_blocks );

    return address;
}

void heap_mark_blocks_free( struct heap *heap,
                            int starting_block )
{
    struct heap_table *table = heap->table;

    for( int free = starting_block; free < ( int ) table->total_entries; free++ )
    {
        HEAP_BLOCK_TABLE_ENTRY entry = table->entries[ free ];
        table->entries[ free ] = HEAP_BLOCK_TABLE_ENTRY_FREE;

        if( !( entry & HEAP_BLOCK_HAS_NEXT ) )
        {
            break;
        }
    }
}

int heap_address_to_block( struct heap *heap,
                           void *address )
{
    return ( ( int ) ( address - heap->start_addr ) ) / OS_HEAP_BLOCK_SIZE;
}

void *heap_malloc( struct heap *heap,
                   size_t size )
{
    size_t aligned_size   = heap_align_value_to_upper( size );
    uint32_t total_blocks = aligned_size / OS_HEAP_BLOCK_SIZE;

    return heap_malloc_blocks( heap, total_blocks );
}

void heap_free( struct heap *heap,
                void *ptr )
{
    heap_mark_blocks_free( heap, heap_address_to_block( heap, ptr ) );
}
