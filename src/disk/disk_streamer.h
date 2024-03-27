#ifndef DISK_STREAMER_H_
#define DISK_STREAMER_H_

#include "disk.h"

struct disk_stream
{
    int position;
    struct disk *disk;
};

struct disk_stream *diskstreamer_new( int disk_id );
int diskstreamer_seek( struct disk_stream *stream,
                       int position );
int diskstreamer_read( struct disk_stream *stream,
                       void *out,
                       int bytes_to_read );
void diskstreamer_close( struct disk_stream *stream );

#endif /* DISK_STREAMER_H_ */
