#ifndef DISK_H_
#define DISK_H_

#include "fs/file.h"

typedef unsigned int OS_DISK_TYPE;

/* represents real physical hard disk */
#define OS_DISK_TYPE_REAL    0

struct disk
{
    OS_DISK_TYPE type;
    int sector_size;
    /* the id of the disk */
    int id;
    struct filesystem *filesystem;
    /* the private data of our filesystem */
    void *fs_private;
};

void disk_search_and_init();
struct disk *disk_get( int index );
int disk_read_block( struct disk *idisk,
                     unsigned int lba,
                     int total_block_to_read,
                     void *buffer );

#endif /* DISK_H_ */
