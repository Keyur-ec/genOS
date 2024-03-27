#include "disk_streamer.h"
#include "memory/heap/kheap.h"
#include "config.h"
#include <stdbool.h>

struct disk_stream *diskstreamer_new( int disk_id )
{
    struct disk *disk = disk_get( disk_id );

    if( !disk )
    {
        return 0;
    }

    struct disk_stream *streamer = kzalloc( sizeof( struct disk_stream ) );

    streamer->position = 0;
    streamer->disk     = disk;
    return streamer;
}

int diskstreamer_seek( struct disk_stream *stream,
                       int position )
{
    stream->position = position;
    return 0;
}

int diskstreamer_read( struct disk_stream *stream,
                       void *out,
                       int bytes_to_read )
{
    int sector        = stream->position / OS_SECTOR_SIZE;
    int offset        = stream->position % OS_SECTOR_SIZE;
    int total_to_read = bytes_to_read;
    bool overflow     = ( offset + total_to_read ) >= OS_SECTOR_SIZE;
    char buffer[ OS_SECTOR_SIZE ];


    if( overflow )
    {
        total_to_read -= ( offset + total_to_read ) - OS_SECTOR_SIZE;
    }

    int res = disk_read_block( stream->disk, sector, 1, buffer );

    if( res < 0 )
    {
        return res;
    }

    for( int idx = 0; idx < total_to_read; idx++ )
    {
        *( char * ) out++ = buffer[ offset + idx ];
    }

    /* adjust the stream */
    stream->position += total_to_read;

    if( overflow )
    {
        res = diskstreamer_read( stream, out, bytes_to_read - total_to_read );
    }

    return res;
}

void diskstreamer_close( struct disk_stream *stream )
{
    kfree( stream );
}
