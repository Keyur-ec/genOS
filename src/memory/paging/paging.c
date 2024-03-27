#include "paging.h"
#include "memory/heap/kheap.h"
#include "status.h"

static uint32_t *current_directory = 0;

void paging_load_directory( uint32_t *directory );

struct paging_chunk *paging_new( uint8_t flags )
{
    uint32_t *directory = kzalloc( sizeof( uint32_t ) * PAGING_TOTAL_ENTRY_PER_TABLE );
    int offset          = 0;

    for( int init = 0; init < PAGING_TOTAL_ENTRY_PER_TABLE; init++ )
    {
        uint32_t *entry = kzalloc( sizeof( uint32_t ) * PAGING_TOTAL_ENTRY_PER_TABLE );

        for( int map = 0; map < PAGING_TOTAL_ENTRY_PER_TABLE; map++ )
        {
            entry[ map ] = ( offset + ( map * PAGING_PAGE_SIZE ) ) | flags;
        }

        offset           += ( PAGING_TOTAL_ENTRY_PER_TABLE * PAGING_PAGE_SIZE );
        directory[ init ] = ( uint32_t ) entry | flags | PAGING_IS_WRITEABLE;
    }

    struct paging_chunk *chunk = kzalloc( sizeof( struct paging_chunk ) );

    chunk->directory_entry = directory;

    return chunk;
}

void paging_free( struct paging_chunk *chunk )
{
    for( int idx = 0; idx < 1024; idx++ )
    {
        uint32_t entry  = chunk->directory_entry[ idx ];
        uint32_t *table = ( uint32_t * ) ( entry & 0xFFFFF000 );
        kfree( table );
    }

    kfree( chunk->directory_entry );
    kfree( chunk );
}

void paging_switch( struct paging_chunk *directory )
{
    paging_load_directory( directory->directory_entry );
    current_directory = directory->directory_entry;
}

uint32_t *paging_chunk_get_directory( struct paging_chunk *chunk )
{
    return chunk->directory_entry;
}

bool paging_is_aligned( void *address )
{
    return ( ( uint32_t ) address % PAGING_PAGE_SIZE ) == 0;
}

int paging_get_indexes( void *virtual_address,
                        uint32_t *directory_index_out,
                        uint32_t *table_index_out )
{
    int res = OS_OK;

    if( !paging_is_aligned( virtual_address ) )
    {
        res = -INVALID_ARGUMENT_ERROR;
        return res;
    }

    *directory_index_out = ( ( uint32_t ) virtual_address / ( PAGING_TOTAL_ENTRY_PER_TABLE * PAGING_PAGE_SIZE ) );

    *table_index_out = ( ( uint32_t ) virtual_address % ( PAGING_TOTAL_ENTRY_PER_TABLE * PAGING_PAGE_SIZE ) / PAGING_PAGE_SIZE );

    return res;
}

int paging_set( uint32_t *directory,
                void *virtual_address,
                uint32_t value )
{
    if( !paging_is_aligned( virtual_address ) )
    {
        return -INVALID_ARGUMENT_ERROR;
    }

    uint32_t directory_index = 0;
    uint32_t table_index     = 0;
    int res = paging_get_indexes( virtual_address, &directory_index, &table_index );

    if( res < 0 )
    {
        return res;
    }

    uint32_t entry  = directory[ directory_index ];
    uint32_t *table = ( uint32_t * ) ( entry & PAGING_ADDRESS_MASK );

    table[ table_index ] = value;

    return res;
}

int paging_map( struct paging_chunk *directory,
                void *virtual_addr,
                void *physical_addr,
                int flags )
{
    if( ( ( unsigned int ) virtual_addr % PAGING_PAGE_SIZE ) || ( ( unsigned int ) physical_addr % PAGING_PAGE_SIZE ) )
    {
        return -INVALID_ARGUMENT_ERROR;
    }

    return paging_set( directory->directory_entry, virtual_addr, ( uint32_t ) physical_addr | flags );
}

int paging_map_range( struct paging_chunk *directory,
                      void *virtual_addr,
                      void *physical_addr,
                      int count,
                      int flags )
{
    int res = OS_OK;

    for( int idx = 0; idx < count; idx++ )
    {
        res = paging_map( directory, virtual_addr, physical_addr, flags );

        if( res < 0 )
        {
            break;
        }

        virtual_addr  += PAGING_PAGE_SIZE;
        physical_addr += PAGING_PAGE_SIZE;
    }

    return res;
}

int paging_map_to( struct paging_chunk *directory,
                   void *virtual_addr,
                   void *physical_addr,
                   void *physical_end_addr,
                   int flags )
{
    int res = OS_OK;

    if( ( uint32_t ) virtual_addr % PAGING_PAGE_SIZE )
    {
        res = -INVALID_ARGUMENT_ERROR;
        return res;
    }

    if( ( uint32_t ) physical_addr % PAGING_PAGE_SIZE )
    {
        res = -INVALID_ARGUMENT_ERROR;
        return res;
    }

    if( ( uint32_t ) physical_end_addr % PAGING_PAGE_SIZE )
    {
        res = -INVALID_ARGUMENT_ERROR;
        return res;
    }

    if( ( uint32_t ) physical_end_addr < ( uint32_t ) physical_addr )
    {
        res = -INVALID_ARGUMENT_ERROR;
        return res;
    }

    uint32_t total_bytes = physical_end_addr - physical_addr;
    int total_pages      = total_bytes / PAGING_PAGE_SIZE;

    res = paging_map_range( directory, virtual_addr, physical_addr, total_pages, flags );

    return res;
}

void *paging_align_address( void *ptr )
{
    if( ( uint32_t ) ptr % PAGING_PAGE_SIZE )
    {
        return ( void * ) ( ( uint32_t ) ptr + PAGING_PAGE_SIZE - ( ( uint32_t ) ptr % PAGING_PAGE_SIZE ) );
    }

    return ptr;
}

void *paging_align_to_lower_page( void *addr )
{
    uint32_t _addr = ( uint32_t ) addr;

    _addr -= ( _addr % PAGING_PAGE_SIZE );

    return ( void * ) _addr;
}

uint32_t paging_get( uint32_t *directory,
                     void *virtual )
{
    uint32_t directory_index = 0;
    uint32_t table_index     = 0;

    paging_get_indexes( virtual, &directory_index, &table_index );
    uint32_t entry  = directory[ directory_index ];
    uint32_t *table = ( uint32_t * ) ( entry & 0xFFFFF000 );

    return table[ table_index ];
}

void *paging_get_physical_address( uint32_t *directory,
                                   void *virtual_address )
{
    void *virtual_address_new = ( void * ) paging_align_to_lower_page( virtual_address );
    void *diffrence           = ( void * ) ( ( uint32_t ) virtual_address - ( uint32_t ) virtual_address_new );

    return ( void * ) ( ( paging_get( directory, virtual_address_new ) & 0xFFFFF000 ) + diffrence );
}
