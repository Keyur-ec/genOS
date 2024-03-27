#ifndef PAGING_H_
#define PAGING_H_

#include <stdint.h>
#include <stdbool.h>

#define PAGING_CACHE_DISABLE            0b00010000
#define PAGING_WRITE_THORUGH            0b00001000
#define PAGING_ACCESS_FROM_ALL          0b00000100
#define PAGING_IS_WRITEABLE             0b00000010
#define PAGING_IS_PRESENT               0b00000001

#define PAGING_TOTAL_ENTRY_PER_TABLE    1024
#define PAGING_PAGE_SIZE                4096

#define PAGING_ADDRESS_MASK             0xFFFFF000

/* 4GB of paging chunk */
struct paging_chunk
{
    uint32_t *directory_entry;
};

struct paging_chunk *paging_new( uint8_t flags );
void paging_free( struct paging_chunk *chunk );
void paging_switch( struct paging_chunk *directory );
uint32_t *paging_chunk_get_directory( struct paging_chunk *chunk );
bool paging_is_aligned( void *address );
int paging_set( uint32_t *directory,
                void *virtual_address,
                uint32_t value );
int paging_map( struct paging_chunk *directory,
                void *virtual_addr,
                void *physical_addr,
                int flags );
int paging_map_range( struct paging_chunk *directory,
                      void *virtual_addr,
                      void *physical_addr,
                      int count,
                      int flags );
int paging_map_to( struct paging_chunk *directory,
                   void *virtual_addr,
                   void *physical_addr,
                   void *physical_end_addr,
                   int flags );
void *paging_align_address( void *ptr );
void *paging_align_to_lower_page( void *addr );
uint32_t paging_get( uint32_t *directory,
                     void *virtual );
void *paging_get_physical_address( uint32_t *directory,
                                   void *virtual_address );

void enable_paging();

#endif /* PAGING_H_ */
