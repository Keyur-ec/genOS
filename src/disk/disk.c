#include "disk.h"
#include "io/io.h"
#include "memory/memory.h"
#include "status.h"
#include "config.h"

struct disk disk;

int disk_read_sector( int lba,
                      int total_block_to_read,
                      void *buffer )
{
    outb( 0x1F6, ( lba >> 24 ) | 0xE0 );
    outb( 0x1F2, total_block_to_read );
    outb( 0x1F3, ( unsigned char ) ( lba & 0xFF ) );
    outb( 0x1F4, ( unsigned char ) ( lba >> 8 ) );
    outb( 0x1F5, ( unsigned char ) ( lba >> 16 ) );
    outb( 0x1F7, 0x20 );

    unsigned short *ptr = ( unsigned short * ) buffer;

    for( int read_bytes = 0; read_bytes < total_block_to_read; read_bytes++ )
    {
        /* wait for the buffer to be ready */
        char chr = insb( 0x1F7 );

        while( !( chr & 0x08 ) )
        {
            chr = insb( 0x1F7 );
        }

        /* copy from hard disk to memory */
        for( int copy_bytes = 0; copy_bytes < 256; copy_bytes++ )
        {
            *ptr = insw( 0x1F0 );
            ptr++;
        }
    }

    return 0;
}

void disk_search_and_init()
{
    bzero( &disk, sizeof( disk ) );
    disk.type        = OS_DISK_TYPE_REAL;
    disk.sector_size = OS_SECTOR_SIZE;
    disk.id          = 0;
    disk.filesystem  = fs_resolve( &disk );
}

struct disk *disk_get( int index )
{
    if( index != 0 )
    {
        return 0;
    }

    return &disk;
}

int disk_read_block( struct disk *idisk,
                     unsigned int lba,
                     int total_block_to_read,
                     void *buffer )
{
    if( idisk != &disk )
    {
        return -IO_ERROR;
    }

    return disk_read_sector( lba, total_block_to_read, buffer );
}
